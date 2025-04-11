// local
#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "audio.hpp"
#include "constants.hpp"
#include "customslider.hpp"
#include "musicitem.hpp"
#include "type_aliases.hpp"
#include "ui_mainwindow.h"

// std
#include <filesystem>

// qt
#include <QFileDialog>
#include <QMimeData>
#include <QSettings>
#include <random>

// audio
#include <taglib/fileref.h>
#include <taglib/tstring.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      audioSink(new QAudioSink()),
      audioBytes(new QByteArray()),
      audioBuffer(new QBuffer(this)),
      progressSlider(new CustomSlider(this)),
      progressTimer(new QPlainTextEdit(this)),
      playButton(new QPushButton(this)),
      trayIcon(new QSystemTrayIcon(this)),
      threadPool(new QThreadPool(this)),
      rng(std::random_device()),
      gen(std::mt19937(rng())),
      playHistory(new IndexSet<u32>()) {
    ui->setupUi(this);
    this->setAcceptDrops(true);

    timer.start(TIMER_UPDATE_INTERVAL_MS);

    progressSlider = ui->progressSlider;
    volumeSlider = ui->volumeSlider;
    progressTimer = ui->progressTimer;

    // Buttons
    playButton = ui->playButton;
    stopButton = ui->stopButton;
    backwardButton = ui->backwardButton;
    forwardButton = ui->forwardButton;
    repeatButton = ui->repeatButton;
    randomButton = ui->randomButton;

    // File menu actions
    fileMenu = ui->menuFile;
    exitAction = ui->actionExit;
    openFileAction = ui->actionOpenFile;
    openFolderAction = ui->actionOpenFolder;
    aboutAction = ui->actionAbout;

    // Taskbar actions
    forwardAction = ui->actionForward;
    backwardAction = ui->actionBackward;
    repeatAction = ui->actionRepeat;
    pauseAction = ui->actionPause;
    resumeAction = ui->actionResume;
    stopAction = ui->actionStop;
    randomAction = ui->actionRandom;

    // Playback controls
    repeat = false;
    random = false;
    forwardDirection = Direction::Forward;
    backwardDirection = Direction::Backward;

    volumeLevel = ui->volumeLevel;

    connect(forwardAction, &QAction::triggered, this,
            [&] { jumpToTrack(forwardDirection); });

    connect(backwardAction, &QAction::triggered, this,
            [&] { jumpToTrack(backwardDirection); });

    connect(repeatAction, &QAction::triggered, this, [&] {
        repeatButton->toggle();
        repeat = !repeat;
    });

    connect(pauseAction, &QAction::triggered, this, [&] {
        audioSink->suspend();
        playButton->setIcon(startIcon);
    });

    connect(resumeAction, &QAction::triggered, this, [&] {
        audioSink->resume();
        playButton->setIcon(pauseIcon);
    });

    connect(stopAction, &QAction::triggered, this, [&] { stopPlayback(); });
    connect(randomAction, &QAction::triggered, this, [&] {
        random = !random;
        randomButton->toggle();

        if (random) {
            backwardDirection = Direction::BackwardRandom;
            forwardDirection = Direction::ForwardRandom;
        } else {
            backwardDirection = Direction::Backward;
            forwardDirection = Direction::Forward;
            playHistory->clear();
        }
    });

    // Eq
    static auto eqEnabled = false;
    static array<f32, EQ_BANDS_N> eqGains = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    // Track tree
    trackTree = ui->tracksTable;
    trackTreeHeader = trackTree->header();

    trackModel = new QStandardItemModel(this);
    trackTreeHeader->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    trackTreeHeader->setSectionsClickable(true);
    trackModel->setHorizontalHeaderLabels({"Title", "Author", "No."});
    trackTree->setModel(trackModel);

    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    trayIcon->show();

    static auto *trayIconMenu = new QMenu(this);

    trayIconMenu->addActions({resumeAction, pauseAction, stopAction,
                              backwardAction, forwardAction, repeatAction,
                              randomAction, exitAction});

    trayIcon->setContextMenu(trayIconMenu);

    static auto *settings =
        new QSettings("com.savannstm.rap", "com.savannstm.rap");

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

    connect(repeatButton, &QPushButton::clicked, this,
            [&] { repeat = !repeat; });
    connect(randomButton, &QPushButton::clicked, this,
            [&] { randomAction->trigger(); });

    connect(trackTree, &QTreeView::pressed, this, [&](const auto &index) {
        switch (QApplication::mouseButtons()) {
            case Qt::RightButton: {
                auto *menu = new QMenu(this);

                QAction *removeAction = menu->addAction("Remove Track");
                QAction *clearAction = menu->addAction("Clear All Tracks");
                QAction *propertiesAction = menu->addAction("Track Properties");

                connect(removeAction, &QAction::triggered, menu, [&] {
                    if (index.isValid()) {
                        trackModel->removeRow(index.row(), index.parent());
                    }
                });

                connect(clearAction, &QAction::triggered, menu, [&] {
                    trackModel->removeRows(0, trackModel->rowCount());
                });

                connect(propertiesAction, &QAction::triggered, menu, [&] {
                    const auto *item = static_cast<MusicItem *>(
                        trackModel->itemFromIndex(index));

                    if (item) {
                        // showTrackProperties(item);
                    }
                });

                menu->exec(QCursor::pos());
                menu->deleteLater();
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

                        playButton->setIcon(pauseIcon);

                        threadPool->start([=, this] { playTrack(path, this); });
                    }

                    currentIndex = index;
                }
                break;
            }
            default:
                break;
        }
    });

    connect(trackTreeHeader, &QHeaderView::sectionClicked, this,
            [&](const u16 index) {
                qDebug() << "clicked";
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
            // TODO: Message
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
                resumeAction->trigger();
            } else {
                pauseAction->trigger();
            }
        }
    });

    connect(stopButton, &QPushButton::clicked, this,
            [&] { stopAction->trigger(); });

    connect(ui->forwardButton, &QPushButton::clicked, this,
            [&] { forwardAction->trigger(); });

    connect(ui->backwardButton, &QPushButton::clicked, this,
            [&] { backwardAction->trigger(); });

    connect(progressSlider, &CustomSlider::mouseMoved, this,
            &MainWindow::updatePlaybackPosition);

    connect(progressSlider, &CustomSlider::mousePressed, this,
            &MainWindow::updatePlaybackPosition);

    connect(this, &MainWindow::playbackFinished, this, [&] {
        if (repeat) {
            audioBuffer->seek(0);
        } else {
            jumpToTrack(forwardDirection);
        }
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        auto *aboutWindow = new AboutWindow(this);
        aboutWindow->show();
    });

    connect(this, &MainWindow::filesDropped, this,
            [&](const vector<path> &files) { fillTable(files); });

    connect(&timer, &QTimer::timeout, this, &MainWindow::updatePositionDisplay);

    connect(volumeSlider, &CustomSlider::mouseMoved, this, [&](const u8 pos) {
        audioVolume = static_cast<f64>(pos) / 100;
        volumeLevel->setPlainText(std::format("{}%", pos).c_str());
        audioSink->setVolume(audioVolume);
    });

    connect(volumeSlider, &CustomSlider::mousePressed, this, [&](const u8 pos) {
        audioVolume = static_cast<f64>(pos) / 100;
        volumeLevel->setPlainText(std::format("{}%", pos).c_str());
        audioSink->setVolume(audioVolume);
    });
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

inline void MainWindow::fillTable(const vector<path> &paths) const {
    const u16 startRow = trackModel->rowCount();

    for (const auto &[idx, filePath] : views::enumerate(paths)) {
        const auto file = TagLib::FileRef(filePath.c_str());
        const auto *tag = file.tag();
        const u16 row = startRow + idx;

        for (u16 col = 0; col < trackModel->columnCount(); col++) {
            // TODO: Check if the same item is already in the table
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

    trackTreeHeader->resizeSections(QHeaderView::ResizeToContents);
    trackModel->sort(2);
}

inline void MainWindow::fillTable(const walk_dir &read_dir) const {
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

inline void MainWindow::fillTable(const path &filePath) const {
    fillTable({filePath});
}

inline void MainWindow::updatePlaybackPosition(const u32 pos) const {
    audioBuffer->seek(static_cast<u32>(pos * audioBytesNum));

    const string timer =
        format("{}/{}", MainWindow::toMinutes(pos / 1000), audioDuration);

    progressTimer->setPlainText(timer.c_str());
}

inline void MainWindow::jumpToTrack(Direction direction) {
    const u16 totalTracks = trackModel->rowCount();
    if (totalTracks == 0) {
        return;
    }

    const auto idx = trackTree->currentIndex();

    u16 nextIdx;
    const u16 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == totalTracks - 1) {
                stopPlayback();
                return;
            }

            nextIdx = row + 1;
            break;
        case Direction::ForwardRandom: {
            playHistory->insert(row);

            auto dist = std::uniform_int_distribution<u16>(0, totalTracks - 1);

            if (playHistory->size() == totalTracks) {
                playHistory->clear();
            }

            do {
                nextIdx = dist(gen);
            } while (playHistory->contains(nextIdx));
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
            if (playHistory->empty()) {
                goto backward;
            }

            const u16 lastPlayed = playHistory->last();
            playHistory->remove(lastPlayed);
            nextIdx = lastPlayed;
            break;
        }
    }

    const auto index = trackTree->model()->index(nextIdx, 0);
    trackTree->setCurrentIndex(index);

    const auto data = trackModel->data(index, Qt::DisplayRole).toString();
    const auto *item =
        static_cast<MusicItem *>(trackModel->itemFromIndex(index));

    const auto path = item->getPath();

    playButton->setIcon(pauseIcon);
    threadPool->start([=, this] { playTrack(path, this); });
}

void MainWindow::updatePositionDisplay() {
    const u32 bytePosition = audioBuffer->pos();
    const u32 bufferSize = audioBuffer->size();

    if (bufferSize > 0) {
        if (previousPosition != bytePosition) {
            const u16 playbackSecond = bytePosition / audioBytesNum;

            if (previousSecond != playbackSecond) {
                const string formatted =
                    format("{}/{}", MainWindow::toMinutes(playbackSecond),
                           audioDuration);
                progressTimer->setPlainText(formatted.c_str());

                progressSlider->setSliderPosition(playbackSecond);
                previousSecond = playbackSecond;
            }

            previousPosition = bytePosition;
        }

        if (audioBuffer->atEnd()) {
            emit playbackFinished();
        }
    }
}

auto MainWindow::toMinutes(const u16 secs) -> string {
    const u8 minutes = secs / 60;
    const u8 seconds = secs % 60;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = format("{}:{}", minutes, secondsPadded);
    return formatted;
}

void MainWindow::stopPlayback() {
    audioSink->stop();
    audioBytes->clear();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressTimer->setPlainText("0:00/0:00");
    playButton->setIcon(startIcon);
}

MainWindow::~MainWindow() { delete ui; }
