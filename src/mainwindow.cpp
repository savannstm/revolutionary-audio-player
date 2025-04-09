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

static f64 DEFAULT_VOLUME = 1.0;

static std::random_device rng;
static std::mt19937 gen = std::mt19937(rng());
static IndexSet<u32> playHistory;

static auto pauseIcon = QIcon::fromTheme("media-playback-pause");
static auto startIcon = QIcon::fromTheme("media-playback-start");
static u32 byteOffset;
static u16 duration;

static QAudioFormat audioFormat;
static vector<i16> samples;
static QBuffer *audioBuffer = new QBuffer();
static QAudioSink *audioSink;
static CustomSlider *progressSlider;
static QPlainTextEdit *progressTimer;
static AudioMonitor *monitor;
static QPushButton *playButton;
static QTreeView *trackTree;
static QHeaderView *trackTreeHeader;
static QStandardItemModel *trackModel;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      trayIcon(new QSystemTrayIcon(this)) {
    ui->setupUi(this);
    this->setAcceptDrops(true);

    audioFormat.setSampleFormat(QAudioFormat::SampleFormat::Int16);

    constexpr u8 bandsNumber = 10;

    audioSink = new QAudioSink();

    progressSlider = ui->progressSlider;
    progressTimer = ui->progressTimer;

    // Buttons
    playButton = ui->playButton;
    static auto *stopButton = ui->stopButton;
    static auto *backwardButton = ui->backwardButton;
    static auto *forwardButton = ui->forwardButton;
    static auto *repeatButton = ui->repeatButton;
    static auto *randomButton = ui->randomButton;

    // File menu actions
    static auto *fileMenu = ui->menuFile;
    static auto *exitAction = ui->actionExit;
    static const auto *openFileAction = ui->actionOpenFile;
    static const auto *openFolderAction = ui->actionOpenFolder;
    static const auto *aboutAction = ui->actionAbout;

    // Taskbar actions
    /*static auto *forwardAction = ui->actionForward;
    static auto *backwardAction = ui->actionBackward;
    static auto *repeatAction = ui->actionRepeat;
    static auto *pauseAction = ui->actionPause;
    static auto *resumeAction = ui->actionResume;
    static auto *stopAction = ui->actionStop;*/

    // Playback controls
    static auto repeat = false;
    static auto random = false;
    static auto forwardDirection = Direction::Forward;
    static auto backwardDirection = Direction::Backward;

    // Eq
    static auto eqEnabled = false;
    static array<f32, bandsNumber> eqGains = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    // Track tree
    trackTree = ui->tracksTable;
    trackTreeHeader = trackTree->header();

    monitor =
        new AudioMonitor(audioBuffer, progressTimer, progressSlider, this);

    trackModel = new QStandardItemModel(this);
    trackTreeHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    trackModel->setHorizontalHeaderLabels({"Title", "Author", "No."});
    trackTree->setModel(trackModel);

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
                this->show();
                this->raise();
                this->setWindowState(this->windowState() &
                                     ~Qt::WindowMinimized);
            default:
                break;
        }
    });

    connect(repeatButton, &QPushButton::clicked, [] { repeat = !repeat; });
    connect(randomButton, &QPushButton::clicked, [] {
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
                auto *menu = new QMenu(this);

                // TODO: Remove row button, properies etc.
                break;
            }
            case Qt::LeftButton: {
                if (index != currentIndex) {
                    const auto data =
                        trackModel->data(index, Qt::DisplayRole).toString();
                    const auto *item = static_cast<MusicItem *>(
                        trackModel->itemFromIndex(index));

                    if (item != nullptr) {
                        const auto path = item->getPath();

                        audioBuffer->close();

                        const auto [metadata, dur] =
                            extractSamples(path, samples);

                        audioBuffer->setData(
                            reinterpret_cast<const char *>(samples.data()),
                            static_cast<i32>(samples.size() * sampleSize));
                        audioBuffer->open(QIODevice::ReadOnly);

                        duration = dur;

                        const auto [channels, sampleRate, frames] = metadata;

                        byteOffset = sampleRate * channels * sampleSize;

                        audioFormat.setSampleRate(sampleRate);
                        audioFormat.setChannelCount(channels);

                        playButton->setIcon(pauseIcon);

                        const auto durationTime =
                            formatSecondsToMinutes(duration);

                        progressSlider->setRange(0, duration);
                        progressTimer->setPlainText(
                            format("0:00/{}", durationTime).c_str());

                        audioSink->stop();

                        audioSink = new QAudioSink(audioFormat, this);

                        monitor->updateDuration(durationTime);
                        monitor->updateBuffer(audioBuffer);

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
                trackModel->sort(index, order);
            });

    connect(openFolderAction, &QAction::triggered, this, [&] {
        const auto directory = QFileDialog::getExistingDirectory(
            this, "Select Directory", lastDir, QFileDialog::ShowDirsOnly);

        if (directory == QDir::rootPath() || directory == QDir::homePath() ||
            directory.isEmpty()) {
            return;
        }

        settings->setValue("lastOpenedDir", directory);

        const auto directoryString = directory.toStdString();

        const auto entries = walk_dir(
            directoryString, fs::directory_options::skip_permission_denied);

        fillTable(entries);
    });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file = QFileDialog::getOpenFileName(this, "Select File");
        fillTable(vector<path>({file.toStdString()}));
    });

    connect(playButton, &QPushButton::clicked, this, [&] {
        if (!audioSink->isNull() && audioSink->bufferSize() != 0) {
            if (audioSink->state() == QtAudio::SuspendedState) {
                audioSink->resume();
                playButton->setIcon(pauseIcon);
            } else {
                audioSink->suspend();
                playButton->setIcon(startIcon);
            }
        }
    });

    connect(stopButton, &QPushButton::clicked, this, [&] {
        audioSink->stop();
        playButton->setIcon(startIcon);
    });

    connect(ui->forwardButton, &QPushButton::clicked, this,
            [&] { jumpToTrack(forwardDirection); });

    connect(ui->backwardButton, &QPushButton::clicked, this,
            [&] { jumpToTrack(backwardDirection); });

    connect(progressSlider, &CustomSlider::mouseMoved, this,
            [&](const u32 pos) { updatePosition(pos); });

    connect(progressSlider, &CustomSlider::mousePressed, this,
            [&](const u32 pos) { updatePosition(pos); });

    connect(monitor, &AudioMonitor::playbackFinished, this, [&] {
        if (repeat) {
            audioBuffer->seek(0);
        } else {
            jumpToTrack(forwardDirection);
        }
    });

    connect(aboutAction, &QAction::triggered, this, [] {
        AboutWindow aboutWindow;
        aboutWindow.show();
    });

    connect(this, &MainWindow::filesDropped, this,
            [&](const vector<path> &files) { fillTable(files); });
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

inline void MainWindow::fillTable(const vector<path> &paths) {
    const u16 startRow = trackModel->rowCount();

    for (const auto &[idx, filePath] : views::enumerate(paths)) {
        const auto file = TagLib::FileRef(filePath.c_str());
        const auto *tag = file.tag();
        const u16 row = startRow + idx;

        for (u16 col = 0; col < 3; col++) {
            auto *item = new MusicItem();
            item->setEditable(false);
            item->setPath(filePath);

            switch (col) {
                case 0: {
                    QString text = QString::fromStdString(
                        (tag != nullptr) && !tag->title().isEmpty()
                            ? tag->title().toCString(true)
                            : filePath.filename().string());
                    item->setText(text);
                    break;
                }
                case 1: {
                    if ((tag != nullptr) && !tag->artist().isEmpty()) {
                        item->setText(QString::fromStdString(
                            tag->artist().toCString(true)));
                    }
                    break;
                }
                case 2: {
                    if (tag != nullptr) {
                        item->setData(tag->track(), Qt::EditRole);
                    }
                    break;
                }
                default:
                    break;
            }

            trackModel->setItem(row, col, item);
        }
    }

    trackModel->sort(2);
}

inline void MainWindow::fillTable(const walk_dir &read_dir) {
    vector<path> files;

    for (const auto &entry : read_dir) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto &filePath = entry.path();

        for (const auto *ext : EXTENSIONS) {
            if (filePath.string().ends_with(ext)) {
                files.emplace_back(filePath);
            }
        }
    }

    fillTable(files);
}

inline void MainWindow::fillTable(const path &filePath) {
    fillTable({filePath});
}

inline void MainWindow::updatePosition(const u32 pos) {
    audioBuffer->seek(static_cast<u32>(pos * byteOffset));

    const string timer = format("{}/{}", formatSecondsToMinutes(pos / 1000),
                                formatSecondsToMinutes(duration));

    progressTimer->setPlainText(timer.c_str());
}

inline void MainWindow::jumpToTrack(Direction direction) {
    const auto idx = trackTree->currentIndex();

    u32 nextIdx;
    const u32 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == trackModel->rowCount() - 1) {
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

    const auto data = trackModel->data(index, Qt::DisplayRole).toString();
    const auto *item =
        static_cast<MusicItem *>(trackModel->itemFromIndex(index));

    const auto path = item->getPath();

    audioBuffer->close();

    const auto [metadata, dur] = extractSamples(path, samples);

    audioBuffer->setData(reinterpret_cast<const char *>(samples.data()),
                         static_cast<i32>(samples.size() * sampleSize));
    audioBuffer->open(QIODevice::ReadOnly);

    duration = dur;

    const auto [channels, sampleRate, frames] = metadata;

    byteOffset = sampleRate * channels * sampleSize;

    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelCount(channels);

    playButton->setIcon(pauseIcon);

    const auto formatted = formatSecondsToMinutes(dur);

    progressSlider->setRange(0, duration);
    progressTimer->setPlainText(std::format("0:00/{}", formatted).c_str());

    audioSink->stop();

    audioSink = new QAudioSink(audioFormat);

    monitor->updateDuration(formatted);
    monitor->updateBuffer(audioBuffer);

    audioSink->start(audioBuffer);
}

MainWindow::~MainWindow() { delete ui; }
