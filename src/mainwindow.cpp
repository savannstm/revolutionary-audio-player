// local
#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "customslider.hpp"
#include "equalizer.hpp"
#include "indexset.hpp"
#include "musicitem.hpp"
#include "progressmonitor.hpp"
#include "type_aliases.hpp"
#include "ui_mainwindow.h"

// std
#include <filesystem>
#include <random>

// qt
#include <QAction>
#include <QApplication>
#include <QAudioSink>
#include <QDebug>
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMediaFormat>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>

// audio
#include <taglib/fileref.h>
#include <taglib/tstring.h>

enum Direction : u8 { Forward, BackwardRandom, Backward, Random };

constexpr static u8 DEFAULT_VOLUME = 100;
constexpr static array<cstr, 7> EXTENSIONS = {".mp3", ".flac", ".opus", ".aac",
                                              ".wav", ".ogg",  ".m4a"};

static std::random_device rng;
static std::mt19937 gen(rng());
static IndexSet<u32> playHistory;

static auto pauseIcon = QIcon::fromTheme("media-playback-pause");
static auto startIcon = QIcon::fromTheme("media-playback-start");
static i32 byteOffset;
static i32 duration;
static auto *audioBuffer = new QBuffer();
static auto *audioSink = new QAudioSink();

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

            auto *content = new QString();

            switch (col) {
                case 0: {
                    const TagLib::String title = tag->title();

                    if (title.isEmpty()) {
                        content = new QString(title.toCWString());
                    } else {
                        content = new QString(path.filename().string().c_str());
                    }
                    break;
                }
                case 1: {
                    const auto *artist = tag->artist().toCWString();
                    content = new QString(artist);
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
                item->setText(*content);
            }

            treeModel->setItem(static_cast<i32>(row), static_cast<i32>(col),
                               item);
        }
    }

    treeModel->sort(2);
}

inline void updatePosition(const u32 pos, QBuffer *audioBuffer,
                           QAudioSink *player, QPlainTextEdit *input,
                           i32 duration, i32 byteOffset) {
    audioBuffer->seek(pos * byteOffset);

    const string timer = formatSecondsToMinutes(pos / 1000) + "/" +
                         formatSecondsToMinutes(duration);

    input->setPlainText(timer.c_str());
}

inline void jumpToTrack(QTreeView *trackTree, QStandardItemModel *treeModel,
                        QAudioSink *sink, QPushButton *playButton,
                        CustomSlider *progressSlider,
                        QPlainTextEdit *progressTimer, AudioMonitor *monitor,
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

    const auto [info, dur, buf] = getAudioFile(path.c_str());

    audioBuffer = buf;
    duration = dur;

    const i32 sampleRate = info.samplerate;
    const i32 channels = info.channels;

    byteOffset = sampleRate * channels * 4;

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    playButton->setIcon(pauseIcon);

    const auto formatted = formatSecondsToMinutes(dur);

    progressSlider->setRange(0, duration);
    progressTimer->setPlainText(("0:00/" + formatted).c_str());

    if (audioSink != nullptr) {
        audioSink->stop();
    }

    audioSink = new QAudioSink(format);

    monitor->updateDuration(formatted);
    monitor->updateBuffer(audioBuffer);

    audioSink->start(audioBuffer);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      tracksModel(new QStandardItemModel(this)),
      trayIcon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);
    constexpr u8 bandsN = 10;

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

    static AudioMonitor monitor(audioBuffer, progressTimer, progressSlider,
                                this);

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

                // TODO: Remove row button, properies etc.
                break;
            }
            case Qt::LeftButton: {
                if (index != currentIndex) {
                    const auto data =
                        tracksModel->data(index, Qt::DisplayRole).toString();
                    const auto *item = static_cast<MusicItem *>(
                        tracksModel->itemFromIndex(index));

                    if (item != nullptr) {
                        const auto path = item->getPath();

                        const auto [info, dur, buf] =
                            eqEnabled ? equalizeAudioFile(
                                            path, "./processed.mp3", eqGains)
                                      : getAudioFile(path);

                        audioBuffer = buf;
                        duration = dur;

                        const i32 sampleRate = info.samplerate;
                        const i32 channels = info.channels;

                        byteOffset = sampleRate * channels * 4;

                        QAudioFormat format;
                        format.setSampleRate(sampleRate);
                        format.setChannelCount(channels);
                        format.setSampleFormat(
                            QAudioFormat::SampleFormat::Float);

                        playButton->setIcon(pauseIcon);

                        const auto formatted = formatSecondsToMinutes(duration);

                        progressSlider->setRange(0, duration);
                        progressTimer->setPlainText(
                            ("0:00/" + formatted).c_str());

                        if (audioSink != nullptr) {
                            audioSink->stop();
                        }

                        audioSink = new QAudioSink(format, this);

                        monitor.updateDuration(formatted);
                        monitor.updateBuffer(audioBuffer);

                        audioSink->start(audioBuffer);
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

        const auto directoryString = directory.toStdU16String();

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
                playButton->setIcon(pauseIcon);
            } else {
                audioSink->suspend();
                playButton->setIcon(startIcon);
            }
        }
    });

    connect(stopButton, &QPushButton::clicked, this, [&]() {
        audioSink->stop();
        playButton->setIcon(startIcon);
    });

    connect(ui->forwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, audioSink, playButton,
                    progressSlider, progressTimer, &monitor,
                    random ? Direction::Random : Direction::Forward);
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, audioSink, playButton,
                    progressSlider, progressTimer, &monitor,
                    random ? Direction::BackwardRandom : Direction::Backward);
    });

    connect(progressSlider, &CustomSlider::mouseMoved, this,
            [&](const u32 pos) {
                updatePosition(pos, audioBuffer, audioSink, progressTimer,
                               duration, byteOffset);
            });
    connect(progressSlider, &CustomSlider::mousePressed, this,
            [&](const u32 pos) {
                updatePosition(pos, audioBuffer, audioSink, progressTimer,
                               duration, byteOffset);
            });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file = QFileDialog::getOpenFileUrl(this, "Select Directory");
        fillTable(tracksModel, vector<path>({file.path().toStdU16String()}));
    });

    connect(&monitor, &AudioMonitor::playbackFinished, this, [&] {
        jumpToTrack(trackTree, tracksModel, audioSink, playButton,
                    progressSlider, progressTimer, &monitor,
                    random ? Direction::Random : Direction::Forward);
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        AboutWindow aboutWindow(this);
        aboutWindow.show();
    });
}

MainWindow::~MainWindow() { delete ui; }
