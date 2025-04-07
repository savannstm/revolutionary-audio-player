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
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTreeView>

// audio
#include <taglib/fileref.h>
#include <taglib/tstring.h>

enum Direction : u8 { Forward, BackwardRandom, Backward, ForwardRandom };

static f64 DEFAULT_VOLUME = 1.0;
static constexpr array<cstr, 9> EXTENSIONS = {
    ".mp3", ".flac", ".opus", ".aac", ".wav", ".ogg", ".m4a", ".mp4", ".mkv"};

static std::random_device rng;
static std::mt19937 gen = std::mt19937(rng());
static IndexSet<u32> playHistory;

static auto pauseIcon = QIcon::fromTheme("media-playback-pause");
static auto startIcon = QIcon::fromTheme("media-playback-start");
static u32 byteOffset;
static u16 duration;
static QBuffer *audioBuffer;
static QAudioSink *audioSink;
static QPlainTextEdit *progressTimer;

static constexpr u8 sampleSize = sizeof(f32);

inline void fillTable(QStandardItemModel *treeModel,
                      const vector<path> &paths) {
    for (u32 row = treeModel->rowCount(); row < paths.size(); row++) {
        const auto &path = paths[row];

        for (u16 col = 0; col < 3; col++) {
            auto *item = new MusicItem();

            const auto file = TagLib::FileRef(path.c_str());
            const auto *tag = file.tag();

            item->setEditable(false);
            item->setPath(path);

            auto *content = new QString();

            switch (col) {
                case 0: {
                    TagLib::String title = "";

                    if (!tag->isEmpty()) {
                        title = tag->title();
                    }

                    content = new QString(
                        !title.isEmpty() ? title.toCString(true)
                                         : path.filename().string().c_str());
                    break;
                }
                case 1: {
                    TagLib::String artist = "";

                    if (!tag->isEmpty()) {
                        artist = tag->artist();
                    }

                    if (!artist.isEmpty()) {
                        content = new QString(tag->artist().toCWString());
                    }
                    break;
                }
                case 2: {
                    if (!tag->isEmpty()) {
                        item->setData(tag->track(), Qt::EditRole);
                    }
                    break;
                }
                default:
                    break;
            }

            if (col != 2) {
                item->setText(*content);
            }

            treeModel->setItem(static_cast<u16>(row), col, item);
        }
    }

    treeModel->sort(2);
}

inline void updatePosition(const u32 pos) {
    audioBuffer->seek(static_cast<u32>(pos * byteOffset));

    const string timer = format("{}/{}", formatSecondsToMinutes(pos / 1000),
                                formatSecondsToMinutes(duration));

    progressTimer->setPlainText(timer.c_str());
}

inline void jumpToTrack(QTreeView *trackTree, QStandardItemModel *treeModel,
                        QPushButton *playButton, CustomSlider *progressSlider,
                        AudioMonitor *monitor, Direction direction) {
    const auto idx = trackTree->currentIndex();

    u32 nextIdx;
    const u32 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == treeModel->rowCount() - 1) {
                audioSink->stop();
                return;
            }

            nextIdx = row + 1;
            break;
        case Direction::ForwardRandom: {
            playHistory.insert(row);

            const u64 totalTracks = trackTree->model()->rowCount();

            auto dist = std::uniform_int_distribution<u32>(0, totalTracks - 1);

            do {
                nextIdx = dist(gen);
            } while (playHistory.contains(nextIdx));
            break;
        }
        case Direction::Backward:
        backward:
            if (row == 0) {
                return;
            }

            nextIdx = row - 1;
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
    }

    const auto index = trackTree->model()->index(static_cast<i32>(nextIdx), 0);
    trackTree->setCurrentIndex(index);

    const auto data = treeModel->data(index, Qt::DisplayRole).toString();
    const auto *item =
        static_cast<MusicItem *>(treeModel->itemFromIndex(index));

    const auto path = item->getPath();

    const auto [info, dur, buf] = getAudioFile(path);

    audioBuffer = buf;
    duration = dur;

    const u16 sampleRate = info.sample_rate;
    const u8 channels = info.ch_layout.nb_channels;

    byteOffset = sampleRate * channels * sampleSize;

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
    this->setAcceptDrops(true);

    constexpr u8 bandsNumber = 10;

    static auto *progressSlider = ui->progressSlider;
    progressTimer = ui->progressTimer;
    static auto *playButton = ui->playButton;
    static auto *stopButton = ui->stopButton;
    static auto *backwardButton = ui->backwardButton;
    static auto *forwardButton = ui->forwardButton;
    static auto *repeatButton = ui->repeatButton;
    static auto *randomButton = ui->randomButton;
    auto kal = nullptr;

    static auto *fileMenu = ui->menuFile;
    static auto *exitAction = ui->actionExit;
    static const auto *openFileAction = ui->actionOpenFile;
    static const auto *openFolderAction = ui->actionOpenFolder;
    static const auto *aboutAction = ui->actionAbout;

    static auto repeat = false;
    static auto random = false;
    static auto forwardDirection = Direction::Forward;
    static auto backwardDirection = Direction::Backward;

    static auto eqEnabled = true;
    static array<f32, bandsNumber> eqGains = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2};

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
            backwardDirection = Direction::Backward;
            forwardDirection = Direction::Forward;
            playHistory.clear();
        } else {
            backwardDirection = Direction::BackwardRandom;
            forwardDirection = Direction::ForwardRandom;
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
                            eqEnabled ? equalizeAudioFile(path, eqGains)
                                      : getAudioFile(path);

                        audioBuffer = buf;
                        duration = dur;

                        const i32 sampleRate = info.sample_rate;
                        const i32 channels = info.ch_layout.nb_channels;

                        byteOffset = sampleRate * channels * sampleSize;

                        QAudioFormat format;
                        format.setSampleRate(sampleRate);
                        format.setChannelCount(channels);
                        format.setSampleFormat(
                            QAudioFormat::SampleFormat::Float);

                        playButton->setIcon(pauseIcon);

                        const auto formatted = formatSecondsToMinutes(duration);

                        progressSlider->setRange(0, duration);
                        progressTimer->setPlainText(
                            std::format("0:00/{}", formatted).c_str());

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
                for (const auto *ext : EXTENSIONS) {
                    const auto &path = entry.path();

                    if (path.string().ends_with(ext)) {
                        vecEntries.emplace_back(path);
                    }
                }
            }
        }

        fillTable(tracksModel, vecEntries);
    });

    connect(playButton, &QPushButton::clicked, this, [&]() {
        if (audioSink->bufferSize() != 0) {
            if (audioSink->state() == QtAudio::SuspendedState) {
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
        jumpToTrack(trackTree, tracksModel, playButton, progressSlider,
                    &monitor, forwardDirection);
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&]() {
        jumpToTrack(trackTree, tracksModel, playButton, progressSlider,
                    &monitor, backwardDirection);
    });

    connect(progressSlider, &CustomSlider::mouseMoved, this,
            [&](const u32 pos) { updatePosition(pos); });

    connect(progressSlider, &CustomSlider::mousePressed, this,
            [&](const u32 pos) { updatePosition(pos); });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file = QFileDialog::getOpenFileName(this, "Select File");
        fillTable(tracksModel, vector<path>({file.toStdU16String()}));
    });

    connect(&monitor, &AudioMonitor::playbackFinished, this, [&] {
        if (repeat) {
            audioBuffer->seek(0);
        } else {
            jumpToTrack(trackTree, tracksModel, playButton, progressSlider,
                        &monitor, forwardDirection);
        }
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        AboutWindow aboutWindow(this);
        aboutWindow.show();
    });

    connect(this, &MainWindow::filesDropped, this,
            [&](const vector<path> &files) { fillTable(tracksModel, files); });
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    this->hide();
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        vector<path> filePaths;

        for (const QUrl &url : event->mimeData()->urls()) {
            const QString filePath = url.toLocalFile();

            if (!filePath.isEmpty()) {
                filePaths.emplace_back(filePath.toStdString());
            }
        }

        emit filesDropped(filePaths);
    }
}

MainWindow::~MainWindow() { delete ui; }
