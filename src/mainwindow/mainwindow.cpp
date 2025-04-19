#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "aliases.hpp"
#include "constants.hpp"
#include "coverwindow.hpp"
#include "customslider.hpp"
#include "enums.hpp"
#include "extractmetadata.hpp"
#include "metadatawindow.hpp"
#include "musicheader.hpp"
#include "musicitem.hpp"
#include "optionmenu.hpp"
#include "randint.hpp"
#include "tominutes.hpp"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    this->setAcceptDrops(true);

    ui->setupUi(this);

    audioStreamer->updateEq(false, eqGains);
    searchMatches.reserve(32);

    audioSink = new QAudioSink();

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
    trackTree->setHeader(trackTreeHeader);

    trackTreeModel->setHorizontalHeaderLabels(headerLabels);
    trackTree->setModel(trackTreeModel);

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
            case RepeatMode::REPEAT_MODES_COUNT:
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
    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        [&](const QSystemTrayIcon::ActivationReason reason) {
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
    }
    );

    connect(
        trackTree,
        &TrackTree::pressed,
        this,
        [&](const QModelIndex& oldIndex, const QModelIndex& newIndex) {
        switch (QApplication::mouseButtons()) {
            case Qt::RightButton: {
                auto* menu = new QMenu(this);

                QAction* removeAction = menu->addAction("Remove Track");
                QAction* clearAction = menu->addAction("Clear All Tracks");
                QAction* propertiesAction = menu->addAction("Track Properties");
                QAction* coverAction = menu->addAction("Show Cover");

                connect(removeAction, &QAction::triggered, menu, [&] {
                    if (newIndex.isValid()) {
                        trackTreeModel->removeRow(
                            newIndex.row(),
                            newIndex.parent()
                        );
                    }
                });

                connect(clearAction, &QAction::triggered, menu, [&] {
                    trackTreeModel->removeRows(0, trackTreeModel->rowCount());
                    trackTreeModel->clearRowMetadata();
                });

                connect(propertiesAction, &QAction::triggered, menu, [&] {
                    const auto* item = static_cast<MusicItem*>(
                        trackTreeModel->itemFromIndex(newIndex)
                    );

                    if (item) {
                        auto* metadataWindow = new MetadataWindow(
                            trackTreeModel->getRowMetadata(newIndex.row()),
                            this
                        );
                        metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                        metadataWindow->show();
                    }
                });

                connect(coverAction, &QAction::triggered, menu, [&] {
                    const auto* item = static_cast<MusicItem*>(
                        trackTreeModel->itemFromIndex(newIndex)
                    );

                    if (item) {
                        auto* coverWindow = new CoverWindow(
                            trackTreeModel->getRowMetadata(newIndex.row()
                            )[Cover],
                            this
                        );
                        coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                        coverWindow->show();
                    }
                });

                menu->exec(QCursor::pos());
                menu->deleteLater();
                break;
            }
            case Qt::LeftButton: {
                if (newIndex != oldIndex) {
                    playTrack(newIndex);
                }
                break;
            }
            default:
                break;
        }
    }
    );

    connect(
        this,
        &MainWindow::trackDataReady,
        this,
        [&](const metadata_array& metadata, const string& path) {
        const u16 row = trackTreeModel->rowCount();
        trackTreeModel->setRowMetadata(row, metadata);

        for (u16 column = 0; column < trackTreeModel->columnCount(); column++) {
            auto* item = new MusicItem();

            if (column == 0) {
                item->setPath(path);
            }

            const string& data =
                trackTreeModel->headerData(column, Qt::Orientation::Horizontal)
                    .toString()
                    .toStdString();

            const TrackProperty property =
                getTrackProperty(data.data(), data.size());

            item->setText(metadata[property].c_str());

            trackTreeModel->setItem(row, column, item);
        }
    }
    );

    connect(this, &MainWindow::trackDataProcessed, this, [&] {
        for (u8 column = 0; column < trackTreeModel->columnCount(); column++) {
            trackTree->resizeColumnToContents(column);
        }
    });

    connect(
        trackTreeHeader,
        &MusicHeader::headerPressed,
        this,
        [&](const u8 index, const Qt::MouseButton button) {
        if (button == Qt::LeftButton) {
            const auto sortOrder = static_cast<Qt::SortOrder>(
                trackTreeHeader->sortIndicatorOrder() ^ Qt::DescendingOrder
            );

            trackTreeModel->sort(index, sortOrder);
            trackTreeHeader->setSortIndicator(index, sortOrder);
        } else if (button == Qt::RightButton) {
            auto* menu = new OptionMenu(this);

            for (const auto& [label, property] : TRACK_PROPERTIES_MAP) {
                auto* action = new QAction(label, menu);
                action->setCheckable(true);

                if (headerLabels.contains(label)) {
                    action->setChecked(true);
                }

                connect(action, &QAction::triggered, this, [&] {
                    const i8 index =
                        static_cast<i8>(headerLabels.indexOf(label));

                    if (index != -1) {
                        headerLabels.removeAt(index);

                        for (u8 column = 0;
                             column < trackTreeModel->columnCount();
                             column++) {
                            const QString columnLabel =
                                trackTreeModel
                                    ->headerData(
                                        column,
                                        Qt::Orientation::Horizontal
                                    )
                                    .toString();

                            if (label == columnLabel) {
                                trackTreeModel->removeColumn(column);
                            }
                        }
                    } else {
                        headerLabels.emplace_back(label);
                        const u8 column = headerLabels.size();

                        for (u16 row = 0; row < trackTreeModel->rowCount();
                             row++) {
                            auto* item = new MusicItem();

                            const metadata_array& metadata =
                                trackTreeModel->getRowMetadata(row);

                            item->setText(metadata[property].c_str());
                            trackTreeModel->setItem(row, column - 1, item);
                        }

                        for (u8 column = 0;
                             column < trackTreeModel->columnCount();
                             column++) {
                            trackTree->resizeColumnToContents(column);
                        }

                        trackTree->setCurrentIndex(trackTree->currentIndex());
                    }

                    trackTreeModel->setHorizontalHeaderLabels(headerLabels);
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
        const QString directory = QFileDialog::getExistingDirectory(
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

        const string directoryString = directory.toStdString();

        const walk_dir entries =
            walk_dir(directoryString, dir_options::skip_permission_denied);

        fillTable(entries);
    });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const QString file = QFileDialog::getOpenFileName(this, "Select File");
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
        aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
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
        volumeGain = static_cast<f32>(volumeSlider->sliderPosition()) / 100.0F;
        audioSink->setVolume(volumeGain);
        volumeLabel->setText(format("{}%", volumeGain).c_str());
    });

    connect(volumeSlider, &CustomSlider::sliderReleased, this, [&] {
        volumeGain = static_cast<f32>(volumeSlider->sliderPosition()) / 100.0F;
        audioSink->setVolume(volumeGain);
        volumeLabel->setText(format("{}%", volumeGain).c_str());
    });

    connect(searchTrackInput, &SearchInput::inputRejected, this, [&] {
        searchTrackInput->clear();
        searchTrackInput->hide();
    });

    connect(searchTrackInput, &SearchInput::returnPressed, this, [&] {
        // TODO: Display matches number
        const string input = searchTrackInput->text().toStdString();

        if (input != previousSearchPrompt) {
            searchMatches.clear();
            searchMatchesPosition = 0;
            previousSearchPrompt = input;
        }

        if (searchMatches.empty()) {
            const usize colonPos = input.find(':');

            string prefix;
            string value;

            bool hasPrefix = false;
            TrackProperty property = TrackProperty::Title;

            if (colonPos != string::npos) {
                prefix = input.substr(0, colonPos);
                value = input.substr(colonPos + 1);
                hasPrefix = true;

                if (prefix == "no") {
                    prefix = "Trer";  // for computing a hash
                }

                property = getTrackProperty(prefix.data(), prefix.size());
            }

            for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
                const metadata_array& metadata =
                    trackTreeModel->getRowMetadata(row);

                string fieldValue;

                if (hasPrefix) {
                    fieldValue = metadata[property];
                } else {
                    fieldValue = metadata[TrackProperty::Title];
                    value = input;
                }

                const auto fieldValueTransformer =
                    views::transform(fieldValue, [](char chr) {
                    return std::tolower(chr);
                });
                const string fieldValueLower = string(
                    fieldValueTransformer.begin(),
                    fieldValueTransformer.end()
                );

                const auto valueTransformer =
                    views::transform(value, [](char chr) {
                    return std::tolower(chr);
                });
                const string valueLower =
                    string(valueTransformer.begin(), valueTransformer.end());

                if (fieldValue.find(value) != string::npos ||
                    fieldValueLower.find(valueLower) != string::npos) {
                    searchMatches.emplace_back(trackTreeModel->index(row, 0));
                }
            }

            if (searchMatches.empty()) {
                searchMatches.clear();
                searchMatchesPosition = 0;
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

    connect(
        audioStreamer,
        &AudioStreamer::progressUpdate,
        this,
        [&](const u16 second) {
        if (!progressSlider->isSliderDown()) {
            const string formatted =
                format("{}/{}", toMinutes(second), audioDuration);

            progressLabel->setText(formatted.c_str());
            progressSlider->setSliderPosition(second);
        }
    }
    );

    connect(audioStreamer, &AudioStreamer::endOfFile, this, [&] {
        if (repeat == RepeatMode::Track) {
            audioStreamer->seekSecond(0);
        } else {
            jumpToTrack(forwardDirection, false);
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
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
        if (trackTreeModel->contains(filePath)) {
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

        for (cstr extension : EXTENSIONS) {
            const string pathString = filePath.string();

            if (pathString.ends_with(extension)) {
                files.emplace_back(pathString);
            }
        }
    }

    threadPool->start([this, files] { fillTable(files); });
}

inline void MainWindow::fillTable(const string& filePath) {
    fillTable({ filePath });
}

inline void MainWindow::updatePlaybackPosition() {
    const u16 second = progressSlider->sliderPosition();
    audioStreamer->seekSecond(second);

    if (audioSink->state() != QtAudio::ActiveState) {
        audioSink->start();
    }

    const string progress =
        format("{}/{}", toMinutes(second / 1000), audioDuration);

    progressLabel->setText(progress.c_str());
}

inline void
MainWindow::jumpToTrack(const Direction direction, const bool clicked) {
    const u16 totalTracks = trackTreeModel->rowCount();
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

    playTrack(index);
}

void MainWindow::stopPlayback() {
    audioStreamer->reset();
    audioSink->stop();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressLabel->setText("0:00/0:00");
    playButton->setIcon(startIcon);
}

void MainWindow::playTrack(const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const string& path = trackTreeModel->getRowPath(index.row());

    audioSink->stop();
    delete audioSink;

    audioStreamer->start(path);

    audioSink = new QAudioSink(audioStreamer->format());

    audioSink->setVolume(volumeGain);

    audioSink->start(audioStreamer);

    const u16 duration = audioStreamer->duration();
    audioDuration = toMinutes(duration);

    progressSlider->setRange(0, duration);
    progressLabel->setText(std::format("0:00/{}", audioDuration).c_str());
}

inline auto MainWindow::getTrackProperty(cstr str, const usize len)
    -> TrackProperty {
    const u8 charA = static_cast<u8>(std::toupper(str[0]));
    const u8 charB = static_cast<u8>(str[1]);
    const u8 charC = static_cast<u8>(str[len - 2]);
    const u8 charD = static_cast<u8>(str[len - 1]);

    const u8 hash = propertyHash(charA, charB, charC, charD);

    QStringList list;

    return TRACK_PROPERTIES[hash];
}
