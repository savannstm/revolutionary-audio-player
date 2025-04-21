#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "aliases.hpp"
#include "audiostreamer.hpp"
#include "audioworker.hpp"
#include "constants.hpp"
#include "coverwindow.hpp"
#include "customslider.hpp"
#include "enums.hpp"
#include "equalizermenu.hpp"
#include "extractmetadata.hpp"
#include "metadatawindow.hpp"
#include "musicheader.hpp"
#include "musicitem.hpp"
#include "optionmenu.hpp"
#include "playlistview.hpp"
#include "randint.hpp"
#include "settingswindow.hpp"
#include "tominutes.hpp"
#include "tracktree.hpp"

#include <QDirIterator>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <qkeysequence.h>

constexpr u8 MINIMUM_MATCHES_NUMBER = 32;
constexpr u8 SEARCH_INPUT_MIN_WIDTH = 64;
constexpr u8 SEARCH_INPUT_HEIGHT = 24;

auto MainWindow::setupUi() -> bool {
    ui->setupUi(this);
    return true;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    connect(eqButton, &QPushButton::toggled, this, [&](const bool checked) {
        // TODO: Make equalizer menu persistent
        if (checked) {
            equalizerMenu = new EqualizerMenu(eqButton, audioWorker);

            if (settings["equalizer"].isArray()) {
                const auto gainsList = settings["equalizer"][2].toArray();
                db_gains_array gains;

                for (const auto [idx, gain] : views::enumerate(gainsList)) {
                    gains[idx] = static_cast<i8>(gain.toInt());
                }

                const auto frequenciesList = settings["equalizer"][2].toArray();
                frequencies_array frequencies;

                for (const auto [idx, frequency] :
                     views::enumerate(frequenciesList)) {
                    frequencies[idx] = static_cast<f32>(frequency.toDouble());
                }

                equalizerMenu->setEqInfo(
                    settings["equalizer"][0].toBool(),
                    settings["equalizer"][1].toInt(),
                    gains,
                    frequencies
                );
            }
        } else {
            const auto [enabled, bandIndex, gains, frequencies] =
                equalizerMenu->getEqInfo();

            QStringList gainsList;

            for (const u8 gain : gains) {
                gainsList.append(QString::number(gain));
            }

            QStringList frequenciesList;

            for (const float frequency : frequencies) {
                frequenciesList.append(
                    QString::number(static_cast<f64>(frequency))
                );
            }

            QJsonArray array;
            array.push_back(enabled);
            array.push_back(bandIndex);
            array.push_back(QJsonArray::fromStringList(gainsList));
            array.push_back(QJsonArray::fromStringList(frequenciesList));

            settings["equalizer"] = array;

            delete equalizerMenu;
        }
    });

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
        QMetaObject::invokeMethod(
            audioWorker,
            &AudioWorker::suspend,
            Qt::QueuedConnection
        );
        playButton->setIcon(startIcon);
    });

    connect(resumeAction, &QAction::triggered, this, [&] {
        QMetaObject::invokeMethod(
            audioWorker,
            &AudioWorker::resume,
            Qt::QueuedConnection
        );
        playButton->setIcon(pauseIcon);
    });

    connect(stopAction, &QAction::triggered, this, [&] {
        stopPlayback();
        trackTree->clearSelection();
    });

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

    connect(exitAction, &QAction::triggered, this, [&] {
        saveSettings();
        QApplication::quit();
    });

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
        this,
        &MainWindow::trackDataReady,
        this,
        [&](TrackTree* tree, const metadata_array& metadata, const QString& path
        ) {
        auto* model = tree->model();

        const u16 row = model->rowCount();
        model->setRowMetadata(row, metadata);

        for (u16 column = 0; column < model->columnCount(); column++) {
            auto* item = new MusicItem();

            if (column == 0) {
                item->setPath(path);
            }

            const QString& propertyString =
                model->headerData(column, Qt::Orientation::Horizontal)
                    .toString();

            const TrackProperty property = getTrackProperty(propertyString);
            item->setText(metadata[property]);
            model->setItem(row, column, item);
        }
    }
    );

    connect(this, &MainWindow::trackDataProcessed, this, [&](TrackTree* tree) {
        for (u8 column = 0; column < tree->model()->columnCount(); column++) {
            tree->resizeColumnToContents(column);
        }
    });

    connect(openFolderAction, &QAction::triggered, this, [&] {
        const QString directory = QFileDialog::getExistingDirectory(
            this,
            "Select Directory",
            lastDir.isEmpty() || !QDir(lastDir).exists() ? QDir::rootPath()
                                                         : lastDir,
            QFileDialog::ShowDirsOnly
        );

        if (directory.isEmpty()) {
            return;
        }

        if (directory == QDir::rootPath() || directory == QDir::homePath()) {
            QMessageBox::warning(
                this,
                "Crazy path!",
                "Cannot search by root or home path, to save your PC. Move music to specific directory or navigate to an existing directory with music."
            );
            return;
        }

        lastDir = directory;

        QDirIterator entries = QDirIterator(
            directory,
            QDir::NoDotAndDotDot | QDir::Files,
            QDirIterator::Subdirectories
        );

        const i32 index = playlistView->addPlaylist(true);
        fillTable(playlistView->playlistTree(index), entries);
    });

    connect(openFileAction, &QAction::triggered, this, [&] {
        const QString file = QFileDialog::getOpenFileName(this, "Select File");
        const i32 index = playlistView->addPlaylist(true);
        fillTable(playlistView->playlistTree(index), { file });
    });

    connect(addFolderAction, &QAction::triggered, this, [&] {
        const QString directory = QFileDialog::getExistingDirectory(
            this,
            "Select Directory",
            lastDir.isEmpty() || !QDir(lastDir).exists() ? QDir::rootPath()
                                                         : lastDir,
            QFileDialog::ShowDirsOnly
        );

        if (directory.isEmpty()) {
            return;
        }

        if (directory == QDir::rootPath() || directory == QDir::homePath()) {
            QMessageBox::warning(
                this,
                "Crazy path!",
                "Cannot search by root or home path, to save your PC. Move music to specific directory or navigate to an existing directory with music."
            );
            return;
        }

        lastDir = directory;

        QDirIterator entries = QDirIterator(
            directory,
            QDir::NoDotAndDotDot | QDir::Files,
            QDirIterator::Subdirectories
        );

        fillTable(trackTree, entries);
    });

    connect(addFileAction, &QAction::triggered, this, [&] {
        const QString file = QFileDialog::getOpenFileName(this, "Select File");
        fillTable(trackTree, { file });
    });

    connect(playButton, &QPushButton::clicked, this, [&] {
        if (audioWorker->playing()) {
            pauseAction->trigger();
        } else {
            resumeAction->trigger();
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
        [&](const QStringList& files) { fillTable(trackTree, files); }
    );

    connect(
        progressSlider,
        &CustomSlider::sliderReleased,
        this,
        &MainWindow::updatePlaybackPosition
    );

    connect(progressSlider, &CustomSlider::valueChanged, this, [&] {
        const QString progress =
            u"%1/%2"_s.arg(toMinutes(progressSlider->value()), audioDuration);

        progressLabel->setText(progress);
    });

    connect(
        volumeSlider,
        &CustomSlider::valueChanged,
        this,
        [&](const u16 value) {
        volumeLabel->setText(format("{}%", value).c_str());
        audioWorker->setVolume(static_cast<f32>(value) / 100.0F);
    }
    );

    connect(volumeSlider, &CustomSlider::sliderReleased, this, [&] {
        const u16 value = volumeSlider->value();
        volumeLabel->setText(format("{}%", value).c_str());
        audioWorker->setVolume(static_cast<f32>(value) / 100.0F);
    });

    connect(searchTrackInput, &CustomInput::inputRejected, this, [&] {
        searchTrackInput->clear();
        searchTrackInput->hide();
    });

    connect(searchTrackInput, &CustomInput::returnPressed, this, [&] {
        // TODO: Display matches number
        QString input = searchTrackInput->text();

        if (input != previousSearchPrompt) {
            searchMatches.clear();
            searchMatches.reserve(MINIMUM_MATCHES_NUMBER);
            searchMatchesPosition = 0;
            previousSearchPrompt = input;
        }

        if (searchMatches.empty()) {
            const isize colonPos = input.indexOf(':');

            QString prefix;
            QString value;

            bool hasPrefix = false;
            TrackProperty property = TrackProperty::Title;

            if (colonPos != -1) {
                prefix = input.slice(0, colonPos);
                value = input.slice(colonPos + 1);
                hasPrefix = true;

                if (prefix == "no") {
                    prefix = "Trer";  // for computing a hash
                }

                property = getTrackProperty(prefix);
            }

            for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
                const metadata_array& metadata =
                    trackTreeModel->getRowMetadata(row);

                QString fieldValue;

                if (hasPrefix) {
                    fieldValue = metadata[property];
                } else {
                    fieldValue = metadata[TrackProperty::Title];
                    value = input;
                }

                const QString fieldValueLower = fieldValue.toLower();
                const QString valueLower = value.toLower();

                if (fieldValue.contains(value) ||
                    fieldValueLower.contains(valueLower)) {
                    searchMatches.emplace_back(trackTreeModel->index(row, 0));
                }
            }

            if (searchMatches.empty()) {
                searchMatches.clear();
                searchMatches.reserve(MINIMUM_MATCHES_NUMBER);
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
        audioWorker,
        &AudioWorker::progressUpdated,
        this,
        [&](const u16 second) {
        if (!progressSlider->isSliderDown()) {
            progressLabel->setText(
                u"%1/%2"_s.arg(toMinutes(second), audioDuration)
            );
            progressSlider->setValue(second);
        }
    }
    );

    connect(audioWorker, &AudioWorker::endOfFile, this, [&] {
        if (repeat == RepeatMode::Track) {
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::seekSecond,
                Qt::QueuedConnection,
                0
            );
        } else {
            jumpToTrack(forwardDirection, false);
        }
    });

    connect(audioWorker, &AudioWorker::duration, this, [&](const u16 seconds) {
        audioDuration = toMinutes(seconds);
        progressSlider->setRange(0, seconds);
        progressLabel->setText(u"0:00/$1"_s.arg(audioDuration));
    });

    connect(settingsAction, &QAction::triggered, this, [&] {
        auto* settingsWindow = new SettingsWindow(this);
        settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
        settingsWindow->show();
    });

    connect(helpAction, &QAction::triggered, this, [&] {
        auto* helpWindow = new SettingsWindow(this);
        helpWindow->setAttribute(Qt::WA_DeleteOnClose);
        helpWindow->show();
    });

    connect(
        playlistView,
        &PlaylistView::currentChanged,
        this,
        [&](const i32 index) {
        trackTree = playlistView->playlistTree(index);
        trackTreeHeader = trackTree->header();
        trackTreeModel = trackTree->model();

        // TODO: Are connects here even okay?

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
                    QAction* propertiesAction =
                        menu->addAction("Track Properties");
                    QAction* coverAction = menu->addAction("Show Cover");

                    // TODO: Remove range of tracks

                    connect(removeAction, &QAction::triggered, menu, [&] {
                        if (newIndex.isValid()) {
                            trackTreeModel->removeRowMetadata(newIndex.row());
                            trackTreeModel->removeRow(newIndex.row());
                        }
                    });

                    connect(clearAction, &QAction::triggered, menu, [&] {
                        trackTreeModel->removeRows(
                            0,
                            trackTreeModel->rowCount()
                        );
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
                                extractCover(trackTreeModel
                                                 ->getRowProperty(
                                                     newIndex.row(),
                                                     TrackProperty::Path
                                                 )
                                                 .toUtf8()
                                                 .constData()),
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
                                auto* item = new MusicItem(
                                    trackTreeModel
                                        ->getRowProperty(row, property)
                                );

                                trackTreeModel->setItem(row, column - 1, item);
                            }

                            for (u8 column = 0;
                                 column < trackTreeModel->columnCount();
                                 column++) {
                                trackTree->resizeColumnToContents(column);
                            }

                            trackTree->setCurrentIndex(trackTree->currentIndex()
                            );
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
    }
    );

    connect(
        playlistTabBar,
        &PlaylistTabBar::renameTabRequested,
        this,
        [&](const i32 index) {
        auto* input = new CustomInput(this);

        input->move(0, 0);
        input->raise();
        input->show();
        input->setFocus();

        connect(input, &CustomInput::returnPressed, input, [=, this] {
            playlistView->setTabText(index, input->text());
            input->deleteLater();
        });
    }
    );

    setAcceptDrops(true);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

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

    repeatButton->setAction(repeatAction);
    randomButton->setAction(randomAction);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText("Track Name");
    searchTrackInput->setMinimumWidth(MINIMUM_MATCHES_NUMBER);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    playlistView->setHeaderLabels(headerLabels);

    parseSettings();
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
        QStringList filePaths;
        filePaths.reserve(MINIMUM_COUNT);

        for (const QUrl& url : event->mimeData()->urls()) {
            if (!url.isEmpty()) {
                filePaths.emplace_back(url.toLocalFile());
            }
        }

        emit filesDropped(filePaths);
    }
}

inline void
MainWindow::fillTable(TrackTree* tree, const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        if (tree->model()->contains(filePath)) {
            continue;
        }

        emit trackDataReady(
            tree,
            extractMetadata(filePath.toUtf8().constData()),
            filePath
        );
    }

    emit trackDataProcessed(tree);
}

inline void MainWindow::fillTable(TrackTree* tree, QDirIterator& iterator) {
    constexpr u8 MINIMUM_COUNT = 128;

    QStringList files;
    files.reserve(MINIMUM_COUNT);

    while (iterator.hasNext()) {
        iterator.next();
        QFileInfo entry = iterator.fileInfo();

        if (!entry.isFile()) {
            continue;
        }

        for (cstr extension : EXTENSIONS) {
            if (entry.fileName().endsWith(extension)) {
                files.emplace_back(entry.filePath());
            }
        }
    }

    threadPool->start([=, this] { fillTable(tree, files); });
}

inline void MainWindow::fillTable(TrackTree* tree, const QString& filePath) {
    fillTable(tree, { filePath });
}

inline void MainWindow::updatePlaybackPosition() {
    const u16 second = progressSlider->value();
    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::seekSecond,
        Qt::QueuedConnection,
        second
    );

    const QString progress =
        u"%1/%2"_s.arg(toMinutes(second / 1000)).arg(audioDuration);

    progressLabel->setText(progress);
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

    const QModelIndex index = trackTreeModel->index(nextIdx, 0);
    trackTree->setCurrentIndex(index);

    playTrack(index);
}

void MainWindow::stopPlayback() {
    audioWorker->stop();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressLabel->setText("0:00/0:00");
    playButton->setIcon(startIcon);
}

void MainWindow::playTrack(const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const u16 row = index.row();

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::start,
        Qt::QueuedConnection,
        trackTreeModel->getRowProperty(row, TrackProperty::Path)
    );

    const QString artistAndTrack = u"%1 - %2"_s.arg(
        trackTreeModel->getRowProperty(row, TrackProperty::Artist),
        trackTreeModel->getRowProperty(row, TrackProperty::Title)
    );

    trackLabel->setText(artistAndTrack);
    this->setWindowTitle(u"RAP: %1"_s.arg(artistAndTrack));
}

inline auto MainWindow::getTrackProperty(const QString& propertyString)
    -> TrackProperty {
    const isize length = propertyString.size();

    const u8 charA = static_cast<u8>(propertyString[0].toUpper().toLatin1());
    const u8 charB = static_cast<u8>(propertyString[1].toLatin1());
    const u8 charC = static_cast<u8>(propertyString[length - 2].toLatin1());
    const u8 charD = static_cast<u8>(propertyString[length - 1].toLatin1());

    const u8 hash = propertyHash(charA, charB, charC, charD);

    return TRACK_PROPERTIES[hash];
}

void MainWindow::saveSettings() {
    settings["lastDir"] = lastDir;
    settings["volume"] = volumeSlider->value();

    QJsonArray tabsArray;

    for (u16 i = 0; i < playlistView->count() - 1; i++) {
        QJsonObject tabObject;

        const QString title = playlistView->tabText(i);
        tabObject.insert("title", title);

        TrackTree* tree = playlistView->playlistTree(i);
        const u32 rowCount = tree->model()->rowCount();

        QStringList paths;
        paths.reserve(rowCount);

        for (u32 row = 0; row < rowCount; row++) {
            paths.append(tree->model()->getRowProperty(row, TrackProperty::Path)
            );
        }

        tabObject["paths"] = QJsonArray::fromStringList(paths);
        tabsArray.append(tabObject);
    }

    settings["tabs"] = tabsArray;

    if (!settingsFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            u"Can't open settings"_s,
            settingsFile.errorString()
        );
        return;
    }

    settingsFile.write(QJsonDocument(settings).toJson());
    settingsFile.close();
}

void MainWindow::parseSettings() {
    if (!settingsFile.open(QIODevice::ReadOnly)) {
        return;
    }

    const QByteArray jsonData = settingsFile.readAll();
    settings = QJsonDocument::fromJson(jsonData, nullptr).object();

    settingsFile.close();

    const auto lastDirVariant = settings["lastDir"];
    const auto volume = settings["volume"];

    if (lastDirVariant.isString()) {
        lastDir = lastDirVariant.toString();
    }

    if (volume.isDouble()) {
        volumeSlider->setValue(volume.toInt());
    }

    const auto tabs = settings["tabs"];

    if (tabs.isArray()) {
        const QJsonArray tabsArray = tabs.toArray();

        for (const auto tabObjectRef : tabsArray) {
            const auto tabObject = tabObjectRef.toObject();

            i32 index;

            const QString title = tabObject.value(u"title"_s).toString();
            index = playlistView->addPlaylist(false, title);

            QJsonArray value = tabObject.value(u"paths"_s).toArray();
            QStringList pathStrings;
            pathStrings.reserve(value.size());

            for (const auto& path : value) {
                pathStrings.append(path.toString());
            }

            fillTable(playlistView->playlistTree(index), pathStrings);
        }

        playlistView->setCurrentIndex(0);
    }
}