// local
#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "equalizer.hpp"
#include "indexset.hpp"
#include "musicitem.hpp"
#include "type_aliases.hpp"
#include "ui_mainwindow.h"

// std
#include <filesystem>
#include <random>

// qt
#include <QAction>
#include <QApplication>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioSink>
#include <QDebug>
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMediaFormat>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTreeView>

// audio
#include <qaudio.h>
#include <qaudiosink.h>
#include <qbytearrayview.h>
#include <qevent.h>
#include <qkeysequence.h>
#include <taglib/fileref.h>

enum Direction : u8 { Forward, BackwardRandom, Backward, Random };

constexpr static u8 DEFAULT_VOLUME = 100;
constexpr static array<cstr, 7> EXTENSIONS = {".mp3", ".flac", ".opus", ".aac",
                                              ".wav", ".ogg",  ".m4a"};

static std::random_device rng;
static std::mt19937 gen(rng());
static IndexSet<u32> playHistory;

inline void fillTable(QStandardItemModel *treeModel,
                      const vector<path> &paths) {
    for (u32 row = 0; row < paths.size(); row++) {
        const auto &path = paths[row];

        for (u32 col = 0; col < 3; col++) {
            auto *item = new MusicItem();

            const TagLib::FileRef file(path.c_str());
            const TagLib::Tag *tag = file.tag();

            item->setEditable(false);
            item->setPath(path);

            QString content;

            switch (col) {
                case 0: {
                    cstr title = tag->title().toCString(true);

                    if (strcmp(title, "\0") != 0) {
                        content = title;
                    } else {
                        content = QString(path.filename().c_str());
                    }
                    break;
                }
                case 1: {
                    cstr artist = tag->artist().toCString(true);
                    content = artist;
                    break;
                }
                case 2: {
                    item->setData(tag->track(), Qt::EditRole);
                    break;
                }
                default:
                    break;
            }

            if (col != 2) {
                item->setText(content);
            }

            treeModel->setItem(static_cast<i32>(row), static_cast<i32>(col),
                               item);
        }
    }

    treeModel->sort(2);
}

auto inline formatSecondsToMinutes(const u64 secs) -> string {
    const u64 minutes = secs / 60;
    const u64 seconds = secs % 60;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = to_string(minutes) + ":" + secondsPadded;
    return formatted;
}

inline void updatePosition(const u32 pos, QMediaPlayer *player,
                           QPlainTextEdit *input) {
    player->setPosition(pos);

    const string timer = formatSecondsToMinutes(pos / 1000) + "/" +
                         formatSecondsToMinutes(player->duration() / 1000);

    input->setPlainText(timer.c_str());
}

inline void jumpToTrack(QTreeView *trackTree, QStandardItemModel *treeModel,
                        QAudioSink *sink, QPushButton *playButton,
                        Direction direction) {
    const auto idx = trackTree->currentIndex();

    u64 nextIdx;
    const u64 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == treeModel->rowCount() - 1) {
                sink->stop();
                return;
            }

            nextIdx = row + 1;
            break;
        case Direction::BackwardRandom: {
            if (playHistory.empty()) {
                goto backward;
            }

            const u64 lastPlayed = playHistory.last();
            playHistory.remove(lastPlayed);
            nextIdx = lastPlayed;
            break;
        }
        case Direction::Backward:
        backward:
            if (row == 0) {
                return;
            }

            nextIdx = row - 1;
            break;
        case Direction::Random:
            playHistory.insert(row);

            const u64 totalTracks = trackTree->model()->rowCount();

            do {
                std::uniform_int_distribution<u64> dist(0, totalTracks - 1);
                nextIdx = dist(gen);
            } while (playHistory.contains(nextIdx));
            break;
    }

    const auto index = trackTree->model()->index(static_cast<i32>(nextIdx), 0);
    trackTree->setCurrentIndex(index);

    const auto data = treeModel->data(index, Qt::DisplayRole).toString();
    const auto *item =
        static_cast<MusicItem *>(treeModel->itemFromIndex(index));

    const auto path = item->getPath();

    const auto [_info, buffer] = readAudioFile(path.c_str());
    auto *qBuffer = makeAudioBuffer(buffer);
    sink = new QAudioSink();

    if (playButton->icon().name() == "media-playback-pause") {
        sink->start(qBuffer);
    }
}

auto endsWith(const char *str, const char *suffix) -> bool {
    usize strLen = strlen(str);
    usize suffixLen = strlen(suffix);

    if (suffixLen > strLen) {
        return false;
    }

    return strcmp(str + (strLen - suffixLen), suffix) == 0;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      tracksModel(new QStandardItemModel(this)),
      trayIcon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);
    constexpr u8 bandsN = 10;

    static QAudioSink *audioSink;
    static auto *progressSlider = ui->progressSlider;
    static auto *progressTimer = ui->progressTimer;
    static auto *playButton = ui->playButton;
    static auto *stopButton = ui->stopButton;
    static auto *backwardButton = ui->backwardButton;
    static auto *forwardButton = ui->forwardButton;
    static auto *repeatButton = ui->repeatButton;
    static auto *randomButton = ui->randomButton;

    static auto *fileMenu = ui->menuFile;
    static auto *exitAction = ui->actionExit;
    static const auto *openFileAction = ui->actionOpenFile;
    static const auto *openFolderAction = ui->actionOpenFolder;
    static const auto *aboutAction = ui->actionAbout;

    static auto repeat = false;
    static auto random = false;

    static auto eqEnabled = false;
    static array<f32, bandsN> eqGains = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2};

    static auto *trackTree = ui->tracksTable;
    static auto *trackTreeHeader = trackTree->header();

    trackTreeHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    tracksModel->setHorizontalHeaderLabels({"Title", "Author", "No."});
    trackTree->setModel(tracksModel);

    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    trayIcon->show();

    static auto *trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayIconMenu);

    static auto *settings = new QSettings("revolutionary-audio-player",
                                          "revolutionary-audio-player");

    static auto lastDir =
        settings->value("lastOpenedDir", QDir::homePath()).toString();

    static QModelIndex currentIndex;

    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    connect(trayIcon, &QSystemTrayIcon::activated, [&](const auto reason) {
        switch (reason) {
            case QSystemTrayIcon::ActivationReason::Trigger:
                if (audioSink->state() == QtAudio::State::ActiveState) {
                    audioSink->suspend();
                } else {
                    audioSink->resume();
                }
                break;
            case QSystemTrayIcon::ActivationReason::DoubleClick:
                this->show();
            default:
                break;
        }
    });

    connect(repeatButton, &QPushButton::clicked, []() { repeat = !repeat; });
    connect(randomButton, &QPushButton::clicked, []() {
        random = !random;

        if (!random) {
            playHistory.clear();
        }
    });

    connect(trackTree, &QTreeView::pressed, [&](const auto &index) {
        switch (QApplication::mouseButtons()) {
            case Qt::RightButton: {
                QMenu menu(this);

                const QAction *action1 = menu.addAction("Option 1");
                const QAction *action2 = menu.addAction("Option 2");

                connect(action1, &QAction::triggered, this,
                        [&]() { tracksModel->removeRow(index.row()); });

                menu.exec(QCursor::pos());
                break;
            }
            case Qt::LeftButton: {
                if (index != currentIndex) {
                    const auto data =
                        tracksModel->data(index, Qt::DisplayRole).toString();
                    const auto *item = static_cast<MusicItem *>(
                        tracksModel->itemFromIndex(index));

                    if (item != nullptr) {
                        auto path = item->getPath();
                        i32 rate;
                        i32 chn;
                        QBuffer *buf;

                        if (eqEnabled) {
                            auto tuple = equalizeAudioFile(
                                path, "./processed.mp3", eqGains);

                            rate = std::get<0>(tuple);
                            chn = std::get<1>(tuple);
                            buf = std::get<2>(tuple);
                        } else {
                            auto [info, buffer] = readAudioFile(path);

                            rate = info.samplerate;
                            chn = info.channels;
                            buf = makeAudioBuffer(buffer);
                        }

                        QAudioFormat format;
                        format.setSampleRate(rate);
                        format.setChannelCount(chn);
                        format.setSampleFormat(
                            QAudioFormat::SampleFormat::Float);

                        audioSink = new QAudioSink(format, this);
                        audioSink->start(buf);
                        playButton->setIcon(QIcon::fromTheme(
                            QIcon::ThemeIcon::MediaPlaybackPause));
                    }

                    currentIndex = index;
                }
                break;
            }
            default:
                break;
        }
    });

    connect(trackTreeHeader, &QHeaderView::sectionClicked,
            [&](const u16 index) {
                const auto order = trackTreeHeader->sortIndicatorOrder() ==
                                           Qt::SortOrder::AscendingOrder
                                       ? Qt::SortOrder::DescendingOrder
                                       : Qt::SortOrder::AscendingOrder;
                tracksModel->sort(index, order);
            });

    connect(openFolderAction, &QAction::triggered, this, [&]() {
        const auto directory = QFileDialog::getExistingDirectory(
            this, "Select Directory", lastDir, QFileDialog::ShowDirsOnly);

        if (directory == QDir::rootPath() || directory == QDir::homePath() ||
            directory == nullptr) {
            return;
        }

        settings->setValue("lastOpenedDir", directory);

        const auto directoryString = directory.toStdWString();

        const auto entries = fs::recursive_directory_iterator(
            directoryString, fs::directory_options::skip_permission_denied);

        vector<path> vecEntries;

        for (const auto &entry : entries) {
            if (entry.is_regular_file()) {
                for (const auto *ext : views::all(EXTENSIONS)) {
                    const auto &path = entry.path();

                    if (QString(path.c_str()).endsWith(ext)) {
                        vecEntries.push_back(path);
                    }
                }
            }
        }

        fillTable(tracksModel, vecEntries);
    });

    connect(playButton, &QPushButton::clicked, this, [&]() {
        if (audioSink->bufferSize() != 0) {
            const auto currentIcon = playButton->icon();

            if (currentIcon.name() == "media-playback-start") {
                audioSink->resume();
                playButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackPause));
            } else {
                audioSink->suspend();
                playButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
            }
        }
    });

    connect(stopButton, &QPushButton::clicked, this, [&]() {
        audioSink->stop();
        playButton->setIcon(
            QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaybackStart));
    });

    connect(ui->forwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, audioSink, playButton,
                    random ? Direction::Random : Direction::Forward);
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, audioSink, playButton,
                    random ? Direction::BackwardRandom : Direction::Backward);
    });

    /*
    connect(player, &QMediaPlayer::mediaStatusChanged, this,
            [&](const QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::LoadedMedia) {
                    const i64 duration = audioSink->();
                    progressSlider->setRange(0, static_cast<i16>(duration));

                    const string timer =
                        "0:00/" + formatSecondsToMinutes(duration / 1000);

                    progressTimer->setPlainText(timer.c_str());
                } else if (status == QMediaPlayer::EndOfMedia) {
                    if (repeat) {
                        player->setPosition(0);
                        player->play();
                    } else {
                        jumpToTrack(
                            trackTree, tracksModel, player, playButton,
                            random ? Direction::Random : Direction::Forward);
                    }
                }
            });

    connect(player, &QMediaPlayer::positionChanged, this, [&](const u16 pos) {
        if (!progressSlider->isSliderDown()) {
            const string timer =
                formatSecondsToMinutes(pos / 1000) + "/" +
                formatSecondsToMinutes(player->duration() / 1000);

            progressTimer->setPlainText(timer.c_str());
            progressSlider->setSliderPosition(pos);
        }
    });*/

    /*
    connect(progressSlider, &CustomSlider::mouseMoved, this,
            [&](const u32 pos) { updatePosition(pos, player, progressTimer); });
    connect(progressSlider, &CustomSlider::mousePressed, this,
            [&](const u32 pos) { updatePosition(pos, player, progressTimer);
    });*/

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file = QFileDialog::getOpenFileUrl(this, "Select Directory");
        fillTable(tracksModel, vector<path>({file.path().toStdWString()}));
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        AboutWindow aboutWindow(this);
        aboutWindow.show();
    });
}

MainWindow::~MainWindow() { delete ui; }
