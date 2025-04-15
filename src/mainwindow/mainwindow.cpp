#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "aliases.hpp"
#include "constants.hpp"
#include "customslider.hpp"
#include "enums.hpp"
#include "extractmetadata.hpp"
#include "musicheader.hpp"
#include "musicitem.hpp"
#include "optionmenu.hpp"
#include "randint.hpp"
#include "tominutes.hpp"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    this->setAcceptDrops(true);

    ui->setupUi(this);

    progressSlider = ui->progressSlider;
    volumeSlider = ui->volumeSlider;
    progressLabel = ui->progressLabel;

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

    volumeLabel = ui->volumeLabel;

    // Track tree
    // TODO: Drag tracks to arrange them as user wants
    trackTree = ui->trackTree;
    trackTreeHeader = new MusicHeader(Qt::Orientation::Horizontal, trackTree);
    trackTreeHeader->setStretchLastSection(true);
    trackTreeHeader->setDragEnabled(true);
    trackTreeHeader->setDragDropMode(QAbstractItemView::DragDrop);
    trackTree->setHeader(trackTreeHeader);

    trackTreeHeader->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);
    trackTreeHeader->setSectionsClickable(true);
    trackModel->setHorizontalHeaderLabels(headerLabels);
    trackTree->setModel(trackModel);

    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    trayIcon->show();

    trayIconMenu->addActions(
        { resumeAction,
          pauseAction,
          stopAction,
          backwardAction,
          forwardAction,
          repeatAction,
          randomAction,
          exitAction }
    );

    trayIcon->setContextMenu(trayIconMenu);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText("Track Name");
    searchTrackInput->setMinimumWidth(64);
    searchTrackInput->setFixedHeight(24);

    repeatButton->setAction(repeatAction);
    randomButton->setAction(randomAction);

    connect(forwardAction, &QAction::triggered, this, [&] {
        jumpToTrack(forwardDirection, true);
    });

    connect(backwardAction, &QAction::triggered, this, [&] {
        jumpToTrack(backwardDirection, true);
    });

    connect(repeatAction, &QAction::triggered, this, [&] {
        switch (repeat) {
            case RepeatMode::Off:
                repeat = RepeatMode::Track;
                break;
            case RepeatMode::Track:
                repeatButton->toggle();
                repeatButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::AudioInputMicrophone)
                );
                repeat = RepeatMode::Playlist;
                break;
            case RepeatMode::Playlist:
                repeatButton->setIcon(
                    QIcon::fromTheme(QIcon::ThemeIcon::MediaPlaylistRepeat)
                );
                repeat = RepeatMode::Off;
                break;
            case RepeatMode::REPEAT_MODES_N:
                break;
        }
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

        if (random) {
            backwardDirection = Direction::BackwardRandom;
            forwardDirection = Direction::ForwardRandom;
        } else {
            backwardDirection = Direction::Backward;
            forwardDirection = Direction::Forward;
            playHistory.clear();
        }
    });

    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    connect(trayIcon, &QSystemTrayIcon::activated, [&](const auto reason) {
        switch (reason) {
            case QSystemTrayIcon::ActivationReason::Trigger:
                this->show();
                this->raise();
                this->setWindowState(
                    this->windowState() & ~Qt::WindowMinimized
                );
            default:
                break;
        }
    });

    connect(trackTree, &QTreeView::pressed, this, [&](const auto& index) {
        switch (QApplication::mouseButtons()) {
            case Qt::RightButton: {
                auto* menu = new QMenu(this);

                QAction* removeAction = menu->addAction("Remove Track");
                QAction* clearAction = menu->addAction("Clear All Tracks");
                QAction* propertiesAction =
                    menu->addAction("Track TRACK_PROPERTIES");

                connect(removeAction, &QAction::triggered, menu, [&] {
                    if (index.isValid()) {
                        trackModel->removeRow(index.row(), index.parent());
                    }
                });

                connect(clearAction, &QAction::triggered, menu, [&] {
                    trackModel->removeRows(0, trackModel->rowCount());
                });

                connect(propertiesAction, &QAction::triggered, menu, [&] {
                    const auto* item =
                        static_cast<MusicItem*>(trackModel->itemFromIndex(index)
                        );

                    if (item) {
                        // TODO
                        // showTrackProperties(item);
                    }
                });

                menu->exec(QCursor::pos());
                menu->deleteLater();
                break;
            }
            case Qt::LeftButton: {
                if (index != currentIndex) {
                    const auto* item =
                        static_cast<MusicItem*>(trackModel->itemFromIndex(index)
                        );

                    const string& path = trackModel->getRowPath(index.row());
                    // TODO: Display cover

                    playTrack(path);

                    currentIndex = index;
                }
                break;
            }
            default:
                break;
        }
    });

    connect(
        this,
        &MainWindow::trackDataReady,
        this,
        [&](const array<string, PROPERTY_COUNT>& metadata, const string& path) {
        const u16 row = trackModel->rowCount();
        trackModel->setRowMetadata(row, metadata);

        for (u16 column = 0; column < trackModel->columnCount(); column++) {
            auto* item = new MusicItem();

            if (column == 0) {
                item->setPath(path);
            }

            const string& data =
                trackModel->headerData(column, Qt::Orientation::Horizontal)
                    .toString()
                    .toStdString();

            const TrackProperty property =
                TRACK_PROPERTIES.find(frozen::string(data))->second;
            item->setText(metadata[property].c_str());

            trackModel->setItem(row, column, item);
        }
    }
    );

    connect(this, &MainWindow::trackDataProcessed, this, [&] {
        trackTree->doItemsLayout();
        trackTree->updateGeometry();
    });

    connect(
        trackTreeHeader,
        &MusicHeader::headerPressed,
        this,
        [&](const u8 index, const Qt::MouseButton button) {
        if (button == Qt::LeftButton) {
            trackModel->sort(
                index,
                static_cast<Qt::SortOrder>(
                    trackTreeHeader->sortIndicatorOrder() ^ Qt::DescendingOrder
                )
            );
        } else if (button == Qt::RightButton) {
            auto* menu = new OptionMenu(this);

            for (const auto& [label, property] : TRACK_PROPERTIES) {
                auto* action = new QAction(label.data(), menu);
                action->setCheckable(true);

                if (headerLabels.contains(label)) {
                    action->setChecked(true);
                }

                connect(action, &QAction::triggered, this, [&] {
                    const i8 index =
                        static_cast<i8>(headerLabels.indexOf(label));

                    if (index != -1) {
                        headerLabels.removeAt(index);

                        for (u8 column = 0; column < trackModel->columnCount();
                             column++) {
                            const auto columnLabel =
                                trackModel
                                    ->headerData(
                                        column,
                                        Qt::Orientation::Horizontal
                                    )
                                    .toString();

                            if (label == columnLabel) {
                                trackModel->removeColumn(column);
                            }
                        }
                    } else {
                        // TODO: Stretch to fit

                        headerLabels.emplace_back(label.data());
                        const u8 column = headerLabels.size();

                        for (u16 row = 0; row < trackModel->rowCount(); row++) {
                            auto* item = new MusicItem();

                            const auto metadata =
                                trackModel->getRowMetadata(row);

                            item->setText(metadata[property].c_str());

                            trackModel->setItem(row, column - 1, item);
                        }

                        trackTree->resizeColumnToContents(true);
                        trackTree->setCurrentIndex(trackTree->currentIndex());
                    }

                    trackModel->setHorizontalHeaderLabels(headerLabels);
                });

                menu->addAction(action);
                action->deleteLater();
            }

            menu->exec(QCursor::pos());
            menu->deleteLater();
        }
    }
    );

    connect(openFolderAction, &QAction::triggered, this, [&] {
        const auto directory = QFileDialog::getExistingDirectory(
            this,
            "Select Directory",
            lastDir,
            QFileDialog::ShowDirsOnly
        );

        if (directory == QDir::rootPath() || directory == QDir::homePath() ||
            directory.isEmpty()) {
            QMessageBox::warning(
                this,
                "Crazy path!",
                "Cannot search by root or home path, to save your PC. Move music to specific directory or navigate to an existing directory with music."
            );
            return;
        }

        settings->setValue("lastOpenedDir", directory);

        const auto directoryString = directory.toStdString();

        const auto entries =
            walk_dir(directoryString, dir_options::skip_permission_denied);

        fillTable(entries);
    });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const auto file = QFileDialog::getOpenFileName(this, "Select File");
        fillTable(vector<string>({ file.toStdString() }));
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

    connect(stopButton, &QPushButton::clicked, this, [&] {
        stopAction->trigger();
    });

    connect(ui->forwardButton, &QPushButton::clicked, this, [&] {
        forwardAction->trigger();
    });

    connect(ui->backwardButton, &QPushButton::clicked, this, [&] {
        backwardAction->trigger();
    });

    connect(aboutAction, &QAction::triggered, this, [&] {
        auto* aboutWindow = new AboutWindow(this);
        aboutWindow->show();
    });

    connect(
        this,
        &MainWindow::filesDropped,
        this,
        [&](const vector<string>& files) { fillTable(files); }
    );

    connect(
        progressSlider,
        &CustomSlider::sliderReleased,
        this,
        &MainWindow::updatePlaybackPosition
    );

    connect(progressSlider, &CustomSlider::valueChanged, this, [&] {
        const string progress = format(
            "{}/{}",
            toMinutes(progressSlider->sliderPosition()),
            audioDuration
        );

        progressLabel->setText(progress.c_str());
    });

    connect(volumeSlider, &CustomSlider::valueChanged, this, [&] {
        const u8 percent = volumeSlider->sliderPosition();
        audioVolume = static_cast<f64>(percent) / MAX_VOLUME;
        audioSink->setVolume(audioVolume);
        volumeLabel->setText(format("{}%", percent).c_str());
    });

    connect(volumeSlider, &CustomSlider::sliderReleased, this, [&] {
        const u8 percent = volumeSlider->sliderPosition();
        audioVolume = static_cast<f64>(percent) / MAX_VOLUME;
        audioSink->setVolume(audioVolume);
        volumeLabel->setText(format("{}%", percent).c_str());
    });

    connect(searchTrackInput, &QLineEdit::returnPressed, searchTrackInput, [&] {
        qDebug() << searchTrackInput->text();
    });

    // TODO: Move to class properties
    static vector<QModelIndex> searchMatches;
    static usize searchMatchesPosition = 0;
    searchMatches.reserve(32);

    // TODO: Listen for Escape key press in searchInput

    connect(searchTrackInput, &QLineEdit::returnPressed, this, [&] {
        // TODO: Search by track name if no prefix, if there's a prefix, search
        // by it. Prefixes follow metadata property names: `artist:`, `no:`,
        // `bitrate:` etc.

        if (searchMatches.empty()) {
            for (u32 row = 0; row < trackModel->rowCount(); row++) {
                const auto& metadata = trackModel->getRowMetadata(row);
                const string& title = metadata[TrackProperty::Title];

                static auto* coverLabel = new QLabel(this);

                const string inputString =
                    searchTrackInput->text().toStdString();
                const string inputStringLowercase =
                    searchTrackInput->text().toLower().toStdString();

                // TODO: Handle multiple matches
                if (title.contains(inputString.c_str()) ||
                    title.contains(inputStringLowercase.c_str())) {
                    searchMatches.emplace_back(trackModel->index(row, 0));
                }
            }

            if (searchMatches.size() == 0) {
                return;
            }

            const QModelIndex& firstMatch =
                searchMatches[searchMatchesPosition];
            trackTree->scrollTo(firstMatch, QTreeView::PositionAtCenter);
            trackTree->setCurrentIndex(firstMatch);
        } else {
            searchMatchesPosition++;

            if (searchMatchesPosition >= searchMatches.size()) {
                searchMatchesPosition = 0;
            }

            const QModelIndex& match = searchMatches[searchMatchesPosition];
            trackTree->scrollTo(match, QTreeView::PositionAtCenter);
            trackTree->setCurrentIndex(match);
        }
    });

    connect(searchShortcut, &QShortcut::activated, this, [&] {
        searchTrackInput->move(
            trackTree->width() - searchTrackInput->width(),
            trackTree->y()
        );

        searchTrackInput->raise();
        searchTrackInput->show();
        searchTrackInput->setFocus();
    });
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    this->hide();
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        constexpr u8 MINIMUM_COUNT = 128;
        vector<string> filePaths;
        filePaths.reserve(MINIMUM_COUNT);

        for (const QUrl& url : event->mimeData()->urls()) {
            if (!url.isEmpty()) {
                const QString filePath = url.toLocalFile();
                filePaths.emplace_back(filePath.toStdString());
            }
        }

        emit filesDropped(filePaths);
    }
}

inline void MainWindow::fillTable(const vector<string>& filePaths) {
    for (const string& filePath : filePaths) {
        if (trackModel->contains(filePath)) {
            continue;
        }

        emit trackDataReady(extractMetadata(filePath), filePath);
    }

    emit trackDataProcessed();
}

inline void MainWindow::fillTable(const walk_dir& read_dir) {
    constexpr u8 MINIMUM_COUNT = 128;
    vector<string> files;
    files.reserve(MINIMUM_COUNT);

    for (const dir_entry& entry : read_dir) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const path& filePath = entry.path();

        for (const char* extension : EXTENSIONS) {
            const string pathString = filePath.string();

            if (pathString.ends_with(extension)) {
                files.emplace_back(pathString);
            }
        }
    }

    QtConcurrent::run([this, files] { fillTable(files); });
}

inline void MainWindow::fillTable(const string& filePath) {
    fillTable({ filePath });
}

inline void MainWindow::updatePlaybackPosition() {
    const u16 second = progressSlider->sliderPosition();
    audioStreamer.seekSecond(second);

    if (audioSink->state() != QtAudio::ActiveState) {
        audioSink->start();
    }

    const string progress =
        format("{}/{}", toMinutes(second / 1000), audioDuration);

    progressLabel->setText(progress.c_str());
}

inline void
MainWindow::jumpToTrack(const Direction direction, const bool clicked) {
    const u16 totalTracks = trackModel->rowCount();
    if (totalTracks == 0) {
        return;
    }

    const QModelIndex idx = trackTree->currentIndex();

    u16 nextIdx;
    const u16 row = idx.row();

    switch (direction) {
        case Direction::Forward:
            if (row == totalTracks - 1) {
                if (repeat == RepeatMode::Playlist || clicked) {
                    nextIdx = 0;
                } else {
                    return;
                }
            } else {
                nextIdx = row + 1;
            }
            break;
        case Direction::ForwardRandom: {
            playHistory.insert(row);

            if (playHistory.size() == totalTracks) {
                playHistory.clear();
            }

            do {
                nextIdx = randint_range(0, totalTracks - 1, randdevice());
            } while (playHistory.contains(nextIdx));
            break;
        }
        case Direction::Backward:
        backward:
            if (row == 0) {
                nextIdx = totalTracks - 1;
            } else {
                nextIdx = row - 1;
            }
            break;
        case Direction::BackwardRandom: {
            if (playHistory.empty()) {
                goto backward;
            }

            const u16 lastPlayed = playHistory.last();
            playHistory.remove(lastPlayed);
            nextIdx = lastPlayed;
            break;
        }
    }

    const QModelIndex index = trackTree->model()->index(nextIdx, 0);
    trackTree->setCurrentIndex(index);

    const auto* item =
        static_cast<MusicItem*>(trackModel->itemFromIndex(index));

    const string& path = trackModel->getRowPath(index.row());
    // TODO: Display cover

    playTrack(path);
}

void MainWindow::stopPlayback() {
    audioStreamer.reset();
    audioSink->stop();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressLabel->setText("0:00/0:00");
    playButton->setIcon(startIcon);
}

void MainWindow::playTrack(const string& path) {
    playButton->setIcon(pauseIcon);

    audioSink->stop();
    audioSink->deleteLater();
    audioSink = nullptr;

    audioStreamer.start(path);

    audioSink = new QAudioSink(audioStreamer.format());
    audioSink->setVolume(audioVolume);

    const u16 duration = audioStreamer.duration();
    audioDuration = toMinutes(duration);

    progressSlider->setRange(0, duration);
    progressLabel->setText(std::format("0:00/{}", audioDuration).c_str());

    audioSink->start(&audioStreamer);
}

void MainWindow::updateProgress() {
    if (!progressSlider->isSliderDown()) {
        const u16 playbackSecond = audioStreamer.second();

        const string formatted =
            format("{}/{}", toMinutes(playbackSecond), audioDuration);

        progressLabel->setText(formatted.c_str());
        progressSlider->setSliderPosition(playbackSecond);
    }
}

void MainWindow::eof() {
    if (repeat == RepeatMode::Track) {
        audioStreamer.seekSecond(0);
    } else {
        jumpToTrack(forwardDirection, false);
    }
}

MainWindow::~MainWindow() {
    delete ui;
}