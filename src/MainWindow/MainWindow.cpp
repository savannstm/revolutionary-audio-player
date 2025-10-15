#include "MainWindow.hpp"

#include "AboutWindow.hpp"
#include "Aliases.hpp"
#include "AudioWorker.hpp"
#include "Constants.hpp"
#include "CoverWindow.hpp"
#include "CustomSlider.hpp"
#include "DurationConversions.hpp"
#include "Enums.hpp"
#include "EqualizerMenu.hpp"
#include "ExtractMetadata.hpp"
#include "HelpWindow.hpp"
#include "IconTextButton.hpp"
#include "MetadataWindow.hpp"
#include "PlaylistView.hpp"
#include "RandInt.hpp"
#include "RapidHasher.hpp"
#include "RepeatRangeMenu.hpp"
#include "Settings.hpp"
#include "SettingsWindow.hpp"
#include "TrackProperties.hpp"
#include "TrackTree.hpp"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QWidgetAction>
#include <QXmlStreamReader>
#include <qnamespace.h>

MainWindow::MainWindow(const QStringList& paths, QWidget* parent) :
    QMainWindow(parent) {
    dockWidget->dockCoverLabel = dockCoverLabel;
    dockWidget->dockMetadataTree = dockMetadataTree;

    connect(
        equalizerButton,
        &QPushButton::toggled,
        this,
        &MainWindow::toggleEqualizerMenu
    );

    connect(actionForward, &QAction::triggered, this, [&] {
        advancePlaylist(forwardDirection);
    });

    connect(actionBackward, &QAction::triggered, this, [&] {
        advancePlaylist(backwardDirection);
    });

    connect(actionRepeat, &QAction::triggered, this, &MainWindow::toggleRepeat);

    connect(actionTogglePlayback, &QAction::triggered, this, [&] {
        togglePlayback();
    });

    connect(actionStop, &QAction::triggered, this, &MainWindow::stopPlayback);

    connect(actionRandom, &QAction::triggered, this, &MainWindow::toggleRandom);

    connect(actionExit, &QAction::triggered, this, &MainWindow::exit);

    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &MainWindow::onTrayIconActivated
    );

    connect(actionOpenFolder, &QAction::triggered, this, [&] {
        addEntry(true, true);
    });

    connect(actionOpenFile, &QAction::triggered, this, [&] {
        addEntry(true, false);
    });

    connect(actionAddFolder, &QAction::triggered, this, [&] {
        addEntry(false, true);
    });

    connect(actionAddFile, &QAction::triggered, this, [&] {
        addEntry(false, false);
    });

    connect(
        actionAbout,
        &QAction::triggered,
        this,
        &MainWindow::showAboutWindow
    );

    connect(
        progressSlider,
        &CustomSlider::sliderReleased,
        this,
        &MainWindow::updatePlaybackPosition
    );

    connect(
        progressSlider,
        &CustomSlider::valueChanged,
        this,
        [&](const u16 value) { MainWindow::updateProgressLabel(value); }
    );

    connect(
        progressSliderCloned,
        &CustomSlider::sliderReleased,
        this,
        &MainWindow::updatePlaybackPosition
    );

    connect(
        progressSliderCloned,
        &CustomSlider::valueChanged,
        this,
        [&](const u16 value) { MainWindow::updateProgressLabel(value); }
    );

    connect(
        progressSlider,
        &CustomSlider::valueChanged,
        progressSliderCloned,
        &CustomSlider::setValue
    );

    connect(
        progressSliderCloned,
        &CustomSlider::valueChanged,
        progressSlider,
        &CustomSlider::setValue
    );

    connect(
        volumeSlider,
        &CustomSlider::valueChanged,
        this,
        &MainWindow::updateVolume
    );

    connect(
        volumeSlider,
        &CustomSlider::valueChanged,
        volumeSliderCloned,
        &CustomSlider::setValue
    );

    connect(
        volumeSliderCloned,
        &CustomSlider::valueChanged,
        volumeSlider,
        &CustomSlider::setValue
    );

    connect(
        searchTrackInput,
        &CustomInput::inputRejected,
        this,
        &MainWindow::cancelSearchInput
    );

    connect(
        searchTrackInput,
        &CustomInput::returnPressed,
        this,
        &MainWindow::searchTrack
    );

    connect(
        searchShortcut,
        &QShortcut::activated,
        this,
        &MainWindow::showSearchInput
    );

    connect(
        audioWorker,
        &AudioWorker::progressUpdated,
        this,
        &MainWindow::onAudioProgressUpdated
    );

    connect(
        audioWorker,
        &AudioWorker::streamEnded,
        this,
        &MainWindow::playNext
    );

    connect(
        actionSettings,
        &QAction::triggered,
        this,
        &MainWindow::showSettingsWindow
    );

    connect(
        actionDocumentation,
        &QAction::triggered,
        this,
        &MainWindow::showHelpWindow
    );

    connect(actionOpenPlaylist, &QAction::triggered, this, [&] {
        importPlaylist(true);
    });

    connect(actionAddPlaylist, &QAction::triggered, this, [&] {
        importPlaylist(false);
    });

    connect(actionExportM3U8Playlist, &QAction::triggered, this, [&] {
        exportPlaylist(PlaylistFileType::M3U);
    });

    connect(actionExportXSPFPlaylist, &QAction::triggered, this, [&] {
        exportPlaylist(PlaylistFileType::XSPF);
    });

    connect(
        playlistView,
        &PlaylistView::indexChanged,
        this,
        &MainWindow::changePlaylist
    );

    connect(
        playlistView,
        &PlaylistView::tabsRemoved,
        this,
        [&](const TabRemoveMode mode, const u8 startIndex, const u8 count) {
        if (playingPlaylist == 0 || playingPlaylist < startIndex) {
            return;
        }

        switch (mode) {
            case Single:
                playingPlaylist -= 1;
                break;
            case ToLeft:
            case Other:
                playingPlaylist = i8(playingPlaylist - count);
                break;
            default:
                break;
        }
    }
    );

    connect(actionRussian, &QAction::triggered, this, [&] {
        retranslate(QLocale::Russian);
    });

    connect(actionEnglish, &QAction::triggered, this, [&] {
        retranslate(QLocale::English);
    });

    connect(
        dockWidget,
        &DockWidget::repositioned,
        this,
        &MainWindow::moveDockWidget
    );

    connect(server, &QLocalServer::newConnection, this, [&] {
        QLocalSocket* clientConnection = server->nextPendingConnection();

        connect(
            clientConnection,
            &QLocalSocket::readyRead,
            [this, clientConnection] {
            const QByteArray data = clientConnection->readAll();
            const QStringList paths =
                QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);

            processArgs(paths);
            focus();

            clientConnection->disconnectFromServer();
        }
        );
    });

    connect(
        new QMediaDevices(this),
        &QMediaDevices::audioOutputsChanged,
        this,
        [&] {
        if (settings->outputDevice == previousDefaultAudioDevice) {
            settings->outputDevice = QMediaDevices::defaultAudioOutput();

            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::changeAudioDevice,
                Qt::QueuedConnection,
                settings->outputDevice,
                playButton->icon().name() == pauseIcon.name()
            );
        }

        previousDefaultAudioDevice = QMediaDevices::defaultAudioOutput();
    }
    );

    connect(
        playlistTabBar,
        &PlaylistTabBar::filesDropped,
        this,
        &MainWindow::dropEvent
    );

    connect(
        dockWidget,
        &DockWidget::resized,
        this,
        &MainWindow::adjustPlaylistView
    );

    ui->controlLayout->insertWidget(
        ui->controlLayout->indexOf(equalizerButton) + 1,
        peakVisualizer
    );

    connect(
        audioWorker,
        &AudioWorker::buildPeaks,
        peakVisualizer,
        &PeakVisualizer::buildPeaks
    );

    connect(
        ui->controlContainer,
        &QWidget::customContextMenuRequested,
        this,
        [&] {
        auto* menu = new QMenu(this);

        const bool visualizerHidden = peakVisualizer->isHidden();
        auto* visualizerAction = menu->addAction(tr("Visualizer"));
        visualizerAction->setCheckable(true);
        visualizerAction->setChecked(!visualizerHidden);

        auto* selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == visualizerAction) {
            peakVisualizer->setHidden(!visualizerHidden);
        }
    }
    );

    connect(
        repeatButton,
        &IconTextButton::customContextMenuRequested,
        this,
        [&](const QPoint& pos) {
        if (repeat != RepeatMode::Track) {
            return;
        }

        if (repeatRangeMenu->isHidden()) {
            repeatRangeMenu->move(
                repeatButton->mapToGlobal(QPoint{ 0, repeatButton->height() })
            );

            repeatRangeMenu->show();
        } else {
            repeatRangeMenu->hide();
        }
    }
    );

    server->listen(u"revolutionary-audio-player-server"_s);

    progressLabelCloned->setText(trackDuration);
    progressSliderCloned->setRange(0, 0);
    volumeSliderCloned->setRange(0, MAX_VOLUME);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

    playButton->setAction(actionTogglePlayback);
    stopButton->setAction(actionStop);
    backwardButton->setAction(actionBackward);
    forwardButton->setAction(actionForward);
    repeatButton->setAction(actionRepeat);
    randomButton->setAction(actionRandom);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText(tr("Track name/property"));
    searchTrackInput->setMinimumWidth(MINIMUM_MATCHES_NUMBER);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    setupTrayIcon();
    initializeSettings();

    equalizerMenu = new EqualizerMenu(this, audioWorker, settings);

    loadSettings();
    processArgs(paths);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::focus() {
    show();
    raise();
    setWindowState(windowState() & ~Qt::WindowMinimized);
}

void MainWindow::processArgs(const QStringList& args) {
    if (!args.empty()) {
        const QFileInfo firstPathInfo = QFileInfo(args[0]);
        QString tabLabel;

        if (settings->flags.playlistNaming == PlaylistNaming::DirectoryName) {
            tabLabel = firstPathInfo.isDir() ? firstPathInfo.fileName()
                                             : firstPathInfo.dir().dirName();
        }

        const u8 index = playlistView->addTab(tabLabel);

        TrackTree* tree = playlistView->tree(index);
        tree->fillTable(args, {}, true);
        changePlaylist(i8(index));

        playlistView->setCurrentIndex(i8(index));

        connect(
            tree,
            &TrackTree::fillingFinished,
            this,
            [&](const bool startPlaying) {
            if (startPlaying) {
                togglePlayback();
            }
        },
            Qt::SingleShotConnection
        );
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QPoint dropPos = event->position().toPoint();
    auto* tree = trackTree;

    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        QStringList filePaths;
        filePaths.reserve(urls.size());

        QStringList dirPaths;
        dirPaths.reserve(urls.size());

        for (const QUrl& url : urls) {
            const QString path = url.toLocalFile();

            if (url.isEmpty()) {
                continue;
            }

            const QFileInfo info = QFileInfo(path);

            if (info.isDir()) {
                dirPaths.append(path);
                continue;
            }

            const QString extension = info.suffix().toLower();

            if (ranges::contains(ALLOWED_FILE_EXTENSIONS, extension)) {
                filePaths.append(path);
            }
        }

        const bool outsideTree =
            tree == nullptr ||
            !QRect(tree->mapToGlobal(QPoint(0, 0)), tree->size())
                 .contains(dropPos);

        if (outsideTree) {
            for (const QString& dirPath : dirPaths) {
                const QFileInfo info = QFileInfo(dirPath);
                auto* newTree =
                    playlistView->tree(playlistView->addTab(info.fileName()));
                newTree->fillTable({ dirPath });
            }
        } else {
            for (const QString& dirPath : dirPaths) {
                tree->fillTable({ dirPath });
            }
        }

        if (!filePaths.isEmpty()) {
            if (outsideTree) {
                const QFileInfo info = QFileInfo(filePaths[0]);
                tree = playlistView->tree(
                    playlistView->addTab(info.dir().dirName())
                );
            }

            tree->fillTable(filePaths);
        }
    }
}

void MainWindow::updatePlaybackPosition() {
    u16 second = progressSlider->value();

    if (CUEOffset != UINT16_MAX) {
        second += CUEOffset;
    }

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::seekSecond,
        Qt::QueuedConnection,
        second
    );

    updateProgressLabel(second / SECOND_MS);
}

void MainWindow::loadPlaylists() {
    QFile playlistsFile =
        QFile(settings->playlistsPath + "/rap-playlists.json");

    if (!playlistsFile.open(QFile::ReadOnly)) {
        return;
    }

    QJsonArray playlistsArray =
        QJsonDocument::fromJson(playlistsFile.readAll()).array();

    for (const auto& playlistObject : playlistsArray) {
        const PlaylistObject playlist =
            PlaylistObject(playlistObject.toObject());

        const u8 index = playlistView->addTab(
            playlist.label,
            playlist.columnProperties,
            playlist.columnStates
        );

        auto* tree = playlistView->tree(index);
        tree->fillTable(playlist.tracks, playlist.cueOffsets);

        if (!playlist.order.empty()) {
            connect(
                tree,
                &TrackTree::fillingFinished,
                this,
                [=](const bool /* ignore */) {
                for (const u16 track : range(0, tree->model()->rowCount())) {
                    tree->model()
                        ->item(track, TrackProperty::Order)
                        ->setData(playlist.order[track], Qt::EditRole);
                }
            },
                Qt::SingleShotConnection
            );
        }

        const QString& imagePath = playlist.backgroundImagePath;

        if (!imagePath.isEmpty()) {
            const QString extension =
                imagePath.sliced(imagePath.lastIndexOf('.') + 1);

            QImage image;

            if (ranges::contains(ALLOWED_MUSIC_FILE_EXTENSIONS, extension)) {
                const auto extracted = extractCover(imagePath.toUtf8().data());

                if (!extracted) {
                    LOG_WARN(extracted.error());
                    return;
                }

                const vector<u8>& coverBytes = extracted.value();

                image.loadFromData(coverBytes.data(), i32(coverBytes.size()));
            } else {
                if (!QFile::exists(imagePath)) {
                    return;
                }

                image.load(imagePath);
            }

            QTimer::singleShot(SECOND_MS, [=, this] {
                playlistView->setBackgroundImage(index, image, imagePath);
            });
        }
    }
}

auto MainWindow::savePlaylists() -> result<bool, QString> {
    QFile playlistsFile =
        QFile(settings->playlistsPath + "/rap-playlists.json");

    if (!playlistsFile.open(QIODevice::WriteOnly)) {
        return err(playlistsFile.errorString());
    }

    QJsonArray playlistsArray;
    const u8 tabCount = playlistView->tabCount();

    for (const u8 tab : range(0, tabCount)) {
        auto* tree = playlistView->tree(tab);
        auto* model = tree->model();

        const u16 rowCount = model->rowCount();

        PlaylistObject playlistObject = PlaylistObject(rowCount);
        playlistObject.label = playlistView->tabLabel(tab);
        playlistObject.backgroundImagePath =
            playlistView->backgroundImagePath(tab);

        for (const u16 row : range(0, rowCount)) {
            const HashMap<TrackProperty, QString> rowMetadata =
                tree->rowMetadata(row);
            const auto& item = model->item(row, 0);

            playlistObject.tracks.append(
                item->data(CUE_FILE_PATH_ROLE).isValid()
                    ? item->data(CUE_FILE_PATH_ROLE).toString()
                    : rowMetadata[TrackProperty::Path]
            );
            playlistObject.order.append(rowMetadata[TrackProperty::Order]);
            playlistObject.cueOffsets.append(
                model->item(row, 0)->data(CUE_OFFSET_ROLE)
            );
        }

        for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
            playlistObject.columnProperties[column] = TrackProperty(
                model->headerData(column, Qt::Horizontal, PROPERTY_ROLE)
                    .toUInt()
            );
            playlistObject.columnStates[column] = tree->isColumnHidden(column);
        }

        playlistsArray.append(playlistObject.stringify());
    }

    playlistsFile.write(
        QJsonDocument(playlistsArray).toJson(QJsonDocument::Compact)
    );
    playlistsFile.close();
    return true;
}

auto MainWindow::saveSettings() -> result<bool, QString> {
    QFile settingsFile = QFile(settings->settingsPath + "/rap-settings.json");

    if (!settingsFile.open(QIODevice::WriteOnly)) {
        return err(settingsFile.errorString());
    }

    settings->currentTab = playlistView->currentIndex();
    settings->volume = volumeSlider->value();

    equalizerMenu->saveSettings();

    DockWidgetPosition dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? Left : Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0 ? Top : Bottom;
    }

    settings->dockWidgetSettings.position = dockWidgetPosition;
    settings->dockWidgetSettings.size = dockWidget->width();
    settings->dockWidgetSettings.imageSize = dockCoverLabel->height();

    peakVisualizer->saveSettings(settings->peakVisualizerSettings);
    settings->save(settingsFile);

    return savePlaylists();
}

void MainWindow::retranslate(const QLocale::Language language) {
    QApplication::removeTranslator(translator);
    delete translator;

    translator = new QTranslator(this);
    const bool success = translator->load(
        QLocale(language),
        "rap",
        ".",
        QApplication::applicationDirPath() + "/translations"
    );

    QApplication::installTranslator(translator);

    ui->retranslateUi(this);

    const u8 tabCount = playlistView->tabCount();
    const QStringList propertyLabelMap = trackPropertiesLabels();

    for (const u8 tab : range(0, tabCount)) {
        auto* tree = playlistView->tree(tab);
        auto* model = tree->model();

        for (const TrackProperty column : DEFAULT_COLUMN_PROPERTIES) {
            model->setHeaderData(
                column,
                Qt::Horizontal,
                propertyLabelMap[column]
            );
            tree->resizeColumnToContents(column);
        }
    }

    equalizerMenu->retranslate();

    emit retranslated();
}

void MainWindow::initializeSettings() {
    // TODO: Check for registry entry with settingsPath
    QFile settingsFile =
        QFile(QApplication::applicationDirPath() + "/rap-settings.json");

    if (!settingsFile.open(QIODevice::ReadOnly)) {
        settings = make_shared<Settings>();
    } else {
        const QByteArray jsonData = settingsFile.readAll();
        settingsFile.close();

        settings = make_shared<Settings>(
            QJsonDocument::fromJson(jsonData, nullptr).object()
        );
    }
}

void MainWindow::loadSettings() {
    volumeSlider->setValue(settings->volume);
    volumeSliderCloned->setValue(settings->volume);

    QString formattedVolume;
    formattedVolume.reserve(4);
    formattedVolume += QString::number(settings->volume);
    formattedVolume += '%';
    volumeLabel->setText(formattedVolume);
    volumeLabelTray->setText(formattedVolume);

    loadPlaylists();
    playlistView->setCurrentIndex(settings->currentTab);
    equalizerMenu->loadSettings();

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::changeAudioDevice,
        Qt::QueuedConnection,
        settings->outputDevice,
        false
    );

    const DockWidgetPosition dockWidgetPosition =
        settings->dockWidgetSettings.position;

    moveDockWidget(dockWidgetPosition);

    mainArea->setSizes({ QWIDGETSIZE_MAX, settings->dockWidgetSettings.size });

    peakVisualizer->loadSettings(settings->peakVisualizerSettings);

    // Update system associations, in case if binary was moved somewhere
    if (settings->flags.contextMenuEntryEnabled) {
        SettingsWindow::createContextMenuEntry();
    }

    if (settings->flags.fileAssociationsEnabled) {
        SettingsWindow::createFileAssociations();
    }

    retranslate(settings->language);
}

void MainWindow::moveDockWidget(const DockWidgetPosition dockWidgetPosition) {
    Qt::Orientation targetMainOrientation;
    Qt::Orientation targetDockOrientation;
    u8 targetIndex;

    switch (dockWidgetPosition) {
        case Left:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 0;
            break;
        case Right:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 1;
            break;
        case Top:
            targetMainOrientation = Qt::Vertical;
            targetDockOrientation = Qt::Horizontal;
            targetIndex = 0;
            break;
        case Bottom:
            targetMainOrientation = Qt::Vertical;
            targetDockOrientation = Qt::Horizontal;
            targetIndex = 1;
            break;
    }

    if (mainArea->orientation() == targetMainOrientation &&
        mainArea->indexOf(dockWidget) == targetIndex) {
        return;
    }

    mainArea->setOrientation(targetMainOrientation);
    dockWidget->setOrientation(targetDockOrientation);

    mainArea->widget(mainArea->indexOf(dockWidget))->setParent(nullptr);
    mainArea->insertWidget(targetIndex, dockWidget);
}

void MainWindow::handleTrackPress(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    switch (QApplication::mouseButtons()) {
        case Qt::RightButton: {
            const QModelIndex currentIndex = trackTree->currentIndex();

            auto* menu = new QMenu(this);

            const QAction* removeAction = menu->addAction(tr("Remove Track"));

            QModelIndexList selectedRows =
                trackTree->selectionModel()->selectedRows();
            const QAction* removeSelectionAction = nullptr;

            if (selectedRows.size() > 2) {
                removeSelectionAction = menu->addAction(tr("Remove Selection"));
            }

            const QAction* clearAction =
                menu->addAction(tr("Clear All Tracks"));
            const QAction* metadataAction =
                menu->addAction(tr("Show Track Metadata"));
            const QAction* coverAction = menu->addAction(tr("Show Cover"));
            const QAction* setPlaylistBackground =
                menu->addAction(tr("Set Playlist Background"));

            const QAction* removePlaylistBackground = nullptr;

            if (playlistView->hasBackgroundImage(
                    playlistView->currentIndex()
                )) {
                removePlaylistBackground =
                    menu->addAction(tr("Remove Playlist Background"));
            }

            const QAction* selectedAction = menu->exec(QCursor::pos());
            delete menu;

            if (selectedAction == nullptr) {
                return;
            }

            if (selectedAction == removeAction) {
                trackTreeModel->removeRow(index.row());

                trackTree->setCurrentIndex(
                    index.row() > currentIndex.row()
                        ? currentIndex
                        : trackTreeModel->index(currentIndex.row() - 1, 0)
                );
            } else if (selectedAction == clearAction) {
                trackTreeModel->removeRows(0, trackTreeModel->rowCount());
            } else if (selectedAction == metadataAction) {
                auto* metadataWindow = new MetadataWindow(
                    trackTree->rowMetadata(index.row()),
                    this
                );
                metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                metadataWindow->show();
            } else if (selectedAction == coverAction) {
                const HashMap<TrackProperty, QString> metadata =
                    trackTree->rowMetadata(index.row());

                auto* coverWindow = new CoverWindow(
                    metadata[TrackProperty::Path],
                    metadata[TrackProperty::Title]
                );
                coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                coverWindow->show();
            } else if (selectedAction == removeSelectionAction) {
                selectedRows.removeOne(trackTree->currentIndex());

                const QModelIndex& startRow = selectedRows.first();
                const isize count = selectedRows.size();

                trackTreeModel->removeRows(startRow.row(), i32(count));

                trackTree->setCurrentIndex(
                    index.row() > currentIndex.row()
                        ? currentIndex
                        : trackTreeModel->index(
                              i32(currentIndex.row() - selectedRows.size()),
                              0
                          )
                );
            } else if (selectedAction == setPlaylistBackground) {
                const QString file = QFileDialog::getOpenFileName(
                    this,
                    tr("Select File"),
                    QString(),
                    tr("Image Files (*.png *.jpg *.jpeg *.webp *.tiff *.bmp)")
                );

                if (file.isEmpty()) {
                    return;
                }

                QImage image;
                image.load(file);

                playlistView->setBackgroundImage(
                    playlistView->currentIndex(),
                    image,
                    file
                );
            } else if (selectedAction == removePlaylistBackground) {
                playlistView->removeBackgroundImage(
                    playlistView->currentIndex()
                );
            }
            break;
        }
        case Qt::LeftButton: {
            break;
        }
        default:
            break;
    }
}

void MainWindow::searchTrack() {
    const QString input = searchTrackInput->text();

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
        TrackProperty property = Title;

        if (colonPos != -1) {
            prefix = input.sliced(0, colonPos);
            value = input.sliced(colonPos + 1);
            hasPrefix = true;

            const auto iter = ranges::find_if(
                SEARCH_PROPERTIES,
                [&](const QStringView property) { return prefix == property; }
            );

            if (iter != SEARCH_PROPERTIES.end()) {
                const usize index =
                    ranges::distance(SEARCH_PROPERTIES.begin(), iter);
                property = as<TrackProperty>(index);
            } else {
                searchTrackInput->clear();
                searchTrackInput->setPlaceholderText(tr("Incorrect property!"));
            }
        }

        const u16 rowCount = trackTreeModel->rowCount();
        for (const u16 row : range(0, rowCount)) {
            const HashMap<TrackProperty, QString> metadata =
                trackTree->rowMetadata(row);
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
                searchMatches.append(trackTreeModel->index(row, 0));
            }
        }

        if (searchMatches.empty()) {
            searchMatches.clear();
            searchMatches.reserve(MINIMUM_MATCHES_NUMBER);
            searchMatchesPosition = 0;
            return;
        }

        const QModelIndex& firstMatch = searchMatches[searchMatchesPosition];
        trackTree->scrollTo(firstMatch, QTreeView::PositionAtCenter);
        trackTree->selectionModel()->select(
            firstMatch,
            QItemSelectionModel::Select | QItemSelectionModel::Rows
        );
    } else {
        searchMatchesPosition++;
        if (searchMatchesPosition >= searchMatches.size()) {
            searchMatchesPosition = 0;
        }

        const QModelIndex& match = searchMatches[searchMatchesPosition];
        trackTree->scrollTo(match, QTreeView::PositionAtCenter);
        trackTree->selectionModel()->select(
            match,
            QItemSelectionModel::Select | QItemSelectionModel::Rows
        );
    }
}

void MainWindow::showSearchInput() {
    if (trackTree == nullptr) {
        return;
    }

    searchTrackInput->move(trackTree->mapToGlobal(
        QPoint(trackTree->width() - searchTrackInput->width(), 0)
    ));
    searchTrackInput->raise();
    searchTrackInput->show();
    searchTrackInput->setFocus();
}

void MainWindow::onAudioProgressUpdated(u16 seconds) {
    const bool draggingSlider =
        progressSlider->isSliderDown() || progressSliderCloned->isSliderDown();

    if (!draggingSlider) {
        if (repeat == RepeatMode::Track) {
            u16 startSecond = repeatRangeMenu->startSecond();
            u16 endSecond = repeatRangeMenu->endSecond();

            if (CUEOffset != UINT16_MAX) {
                startSecond += CUEOffset;
                endSecond += CUEOffset;
            }

            if (seconds < startSecond && seconds < startSecond - 1) {
                audioWorker->seekSecond(startSecond);
            }

            if (seconds > endSecond) {
                audioWorker->seekSecond(startSecond);
            }
        }

        const QString& duration =
            trackDuration.slice(trackDuration.indexOf('/') + 1);

        const u16 secsDuration = timeToSecs(duration);

        if (CUEOffset != UINT16_MAX) {
            seconds -= CUEOffset;

            if (seconds == secsDuration + 1) {
                playNext();
            }
        }

        updateProgressLabel(seconds, duration);
        progressSlider->setValue(seconds);
    }
}

void MainWindow::playNext() {
    switch (repeat) {
        case Track:
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::seekSecond,
                Qt::QueuedConnection,
                0
            );
            break;
        default:
            advancePlaylist(forwardDirection);
            break;
    }
}

void MainWindow::showSettingsWindow() {
    auto* settingsWindow = new SettingsWindow(settings, this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->show();

    connect(
        settingsWindow,
        &SettingsWindow::audioDeviceChanged,
        this,
        [&](QAudioDevice device) {
        QMetaObject::invokeMethod(
            audioWorker,
            &AudioWorker::changeAudioDevice,
            Qt::QueuedConnection,
            device,
            playButton->icon().name() == pauseIcon.name()
        );
    }
    );
}

void MainWindow::showHelpWindow() {
    auto* helpWindow = new HelpWindow(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->show();
}

void MainWindow::addEntry(const bool createNewTab, const bool isFolder) {
    const QString searchDirectory =
        settings->lastOpenedDirectory.isEmpty() ||
                !QFile::exists(settings->lastOpenedDirectory)
            ? QDir::rootPath()
            : settings->lastOpenedDirectory;
    QString path;

    if (isFolder) {
        path = QFileDialog::getExistingDirectory(
            this,
            tr("Select Directory"),
            searchDirectory,
            QFileDialog::ShowDirsOnly
        );

        if (path.isEmpty()) {
            return;
        }

        if (path == QDir::rootPath() || path == QDir::homePath()) {
            QMessageBox::warning(
                this,
                tr("Invalid Path"),
                tr("Cannot search by root or home path.")
            );
            return;
        }

        settings->lastOpenedDirectory = path;
    } else {
        path = QFileDialog::getOpenFileName(
            this,
            tr("Select File"),
            searchDirectory,
            constructAudioFileFilter()
        );

        if (path.isEmpty()) {
            return;
        }
    }

    auto* tree = trackTree;

    if (createNewTab || tree == nullptr) {
        const u8 index = playlistView->addTab(
            settings->flags.playlistNaming == DirectoryName
                ? QFileInfo(path).fileName()
                : QString()
        );
        tree = playlistView->tree(index);
    }

    tree->fillTable({ path });
}

void MainWindow::showAboutWindow() {
    auto* aboutWindow = new AboutWindow(this);
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
}

inline void
MainWindow::updateProgressLabel(const u16 second, const QString& duration) {
    const QString time = secsToMins(second);

    if (second != 0 &&
        time == trackDuration.left(trackDuration.indexOf(u'/'))) {
        return;
    }

    QString newTrackDuration;
    newTrackDuration.reserve(5 + 1 + 5);
    newTrackDuration += time;
    newTrackDuration += '/';
    newTrackDuration +=
        duration.isEmpty() ? trackDuration.slice(trackDuration.indexOf('/') + 1)
                           : duration;
    trackDuration = newTrackDuration;

    progressLabel->setText(trackDuration);
    progressLabelCloned->setText(trackDuration);
}

void MainWindow::updateVolume(const u16 value) {
    QString formattedVolume;
    formattedVolume.reserve(3);
    formattedVolume += QString::number(value);
    formattedVolume += '%';

    volumeLabel->setText(formattedVolume);
    volumeLabelCloned->setText(formattedVolume);
    audioWorker->setVolume(f32(value) / 100.0F);
}

void MainWindow::cancelSearchInput() {
    searchTrackInput->clear();
    searchTrackInput->hide();
}

void MainWindow::toggleEqualizerMenu(const bool checked) {
    if (checked) {
        equalizerMenu->move(
            equalizerButton->mapToGlobal(QPoint(0, equalizerButton->height()))
        );
    }

    equalizerMenu->setHidden(!checked);
}

void MainWindow::toggleRepeat() {
    switch (repeat) {
        case RepeatMode::Off:
            repeat = RepeatMode::Playlist;

            repeatRangeMenu->hide();
            repeatButton->setChecked(true);
            actionRepeat->setText(tr("Repeat (Playlist)"));
            break;
        case RepeatMode::Playlist:
            repeat = RepeatMode::Track;

            repeatButton->setChecked(true);
            repeatButton->setText(u"1"_s);
            actionRepeat->setText(tr("Repeat (Track)"));
            actionRepeat->setChecked(true);
            break;
        case RepeatMode::Track:
            repeat = RepeatMode::Off;

            repeatRangeMenu->hide();
            repeatButton->setChecked(false);
            repeatButton->setText(QString());
            actionRepeat->setText(tr("Repeat"));
            break;
        default:
            break;
    }
}

void MainWindow::toggleRandom() {
    random = !random;

    if (random) {
        backwardDirection = Direction::BackwardRandom;
        forwardDirection = Direction::ForwardRandom;
    } else {
        backwardDirection = Direction::Backward;
        forwardDirection = Direction::Forward;
        playHistory.clear();
    }
}

void MainWindow::exit() {
    const auto saveResult = saveSettings();

    if (saveResult) {
        QApplication::quit();
    } else {
        QMessageBox messageBox(this);
        messageBox.setWindowTitle(tr("Save Failed!"));
        messageBox.setText(saveResult.error());
        messageBox.setIcon(QMessageBox::Warning);

        QPushButton* tryAgainButton =
            messageBox.addButton(tr("Try Again"), QMessageBox::AcceptRole);
        QPushButton* cancelButton =
            messageBox.addButton(tr("Cancel"), QMessageBox::RejectRole);

        messageBox.exec();

        if (messageBox.clickedButton() == tryAgainButton) {
            exit();
        }
    }
}

void MainWindow::onTrayIconActivated(
    const QSystemTrayIcon::ActivationReason reason
) {
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        focus();
    }
}

void MainWindow::setupTrayIcon() {
    trayIcon->setIcon(
        QIcon(QApplication::applicationDirPath() + "/icons/rap-logo.png")
    );
    trayIcon->show();

    auto* volumeSliderAction = new QWidgetAction(trayIconMenu);

    auto* volumeSliderContainer = new QWidget(trayIconMenu);
    auto* volumeSliderLayout = new QHBoxLayout(volumeSliderContainer);

    volumeSliderLayout->addWidget(volumeSliderCloned);
    volumeSliderLayout->addWidget(volumeLabelCloned);

    volumeSliderAction->setDefaultWidget(volumeSliderContainer);

    auto* progressSliderAction = new QWidgetAction(trayIconMenu);

    auto* progressSliderContainer = new QWidget(trayIconMenu);
    auto* progressSliderLayout = new QHBoxLayout(progressSliderContainer);

    progressSliderLayout->addWidget(progressSliderCloned);
    progressSliderLayout->addWidget(progressLabelCloned);

    progressSliderAction->setDefaultWidget(progressSliderContainer);

    trayIconMenu->addActions(
        { volumeSliderAction,
          progressSliderAction,
          actionTogglePlayback,
          actionStop,
          actionBackward,
          actionForward,
          actionRepeat,
          actionRandom,
          actionExit }
    );

    trayIcon->setContextMenu(trayIconMenu);
}

auto MainWindow::changePlaylist(const i8 index) -> bool {
    // No playlists left.
    if (index == -1) {
        playingPlaylist = -1;
        trackTree = nullptr;
        trackTreeHeader = nullptr;
        trackTreeModel = nullptr;
        return false;
    }

    if (index > playlistView->tabCount() - 1) {
        return false;
    }

    trackTree = playlistView->tree(index);
    trackTreeHeader = trackTree->header();
    trackTreeModel = trackTree->model();

    connect(
        trackTree,
        &TrackTree::trackSelected,
        this,
        &MainWindow::selectTrack,
        Qt::UniqueConnection
    );

    connect(
        trackTree,
        &TrackTree::pressed,
        this,
        &MainWindow::handleTrackPress,
        Qt::UniqueConnection
    );

    return true;
}

void MainWindow::selectTrack(const i32 oldRow, const u16 newRow) {
    if (oldRow != newRow) {
        auto* tree = playlistView->tree(playingPlaylist);

        if (tree != nullptr) {
            tree->deselect(oldRow);
        }

        trackTree->setCurrentIndex(trackTreeModel->index(newRow, 0));
        playingPlaylist = playlistView->currentIndex();
        playTrack(trackTree, trackTreeModel->index(newRow, 0));
    }

    trackTree->clearSelection();
}

void MainWindow::playTrack(TrackTree* tree, const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const HashMap<TrackProperty, QString> metadata =
        tree->rowMetadata(index.row());

    const bool playingCUE =
        metadata[TrackProperty::Format].toLower().endsWith(EXT_CUE);

    if (playingCUE) {
        CUEOffset =
            tree->model()->item(index.row(), 0)->data(CUE_OFFSET_ROLE).toUInt();
    }

    togglePlayback(
        metadata[TrackProperty::Path],
        playingCUE ? CUEOffset : UINT16_MAX
    );

    tree->model()->itemFromIndex(index)->setData(startIcon, Qt::DecorationRole);

    QString artistAndTrack;
    artistAndTrack.reserve(isize(
        metadata[TrackProperty::Artist].size() + (sizeof(" - ") - 1) +
        metadata[TrackProperty::Title].size()
    ));
    artistAndTrack += metadata[TrackProperty::Artist];
    artistAndTrack += u" - ";
    artistAndTrack += metadata[TrackProperty::Title];
    trackLabel->setText(artistAndTrack);

    QString windowTitle;
    windowTitle.reserve(isize(sizeof("RAP: ") - 1) + artistAndTrack.size());
    windowTitle += u"RAP: ";
    windowTitle += artistAndTrack;
    setWindowTitle(windowTitle);

    for (const u8 property : range(1, TRACK_PROPERTY_COUNT - 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(property - 1, 0))
            ->setText(1, metadata[as<TrackProperty>(property)]);
    }

    QPixmap cover;
    QString path;

    if (settings->flags.prioritizeExternalCover) {
        const QDir fileDirectory = metadata[TrackProperty::Path].sliced(
            0,
            metadata[TrackProperty::Path].lastIndexOf('/')
        );

        const QStringList entries =
            fileDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot);

        for (const QString& entry : entries) {
            const QString extension =
                entry.sliced(entry.lastIndexOf('.') + 1).toLower();

            if (!ranges::contains(IMAGE_EXTENSIONS, extension)) {
                continue;
            }

            const QString prefix = entry.sliced(0, 5).toLower();

            if (prefix != u"cover"_qsv) {
                continue;
            };

            path = fileDirectory.filePath(entry);
            cover.load(path);
            break;
        }
    }

    if (!settings->flags.prioritizeExternalCover || path.isEmpty()) {
        const auto extracted =
            extractCover(metadata[TrackProperty::Path].toUtf8().data());

        if (!extracted) {
            LOG_WARN(extracted.error());
        } else {
            const vector<u8>& coverBytes = extracted.value();
            cover.loadFromData(coverBytes.data(), coverBytes.size());
            path = metadata[TrackProperty::Path];
        }
    }

    if (settings->flags.autoSetBackground) {
        const QImage image = cover.toImage();
        playlistView->setBackgroundImage(playingPlaylist, image, path);
    }

    const u16 seconds = timeToSecs(metadata[TrackProperty::Duration]);
    repeatRangeMenu->setDuration(seconds);
    repeatRangeMenu->setStartSecond(0);
    progressSlider->setRange(0, seconds);
    progressSliderCloned->setRange(0, seconds);
    updateProgressLabel(0, metadata[TrackProperty::Duration]);

    dockCoverLabel->clear();
    dockCoverLabel->setPixmap(cover);
}

void MainWindow::togglePlayback(const QString& path, const u16 startSecond) {
    if (!path.isEmpty()) {
        QMetaObject::invokeMethod(
            audioWorker,
            &AudioWorker::start,
            Qt::QueuedConnection,
            path,
            startSecond
        );
        playButton->setIcon(pauseIcon);
        playButton->setToolTip(tr("Pause"));
        actionTogglePlayback->setText(tr("Pause"));
    } else {
        const QtAudio::State state = audioWorker->state();

        if (state == QtAudio::ActiveState) {
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::suspend,
                Qt::QueuedConnection
            );
            playButton->setIcon(startIcon);
            playButton->setToolTip(tr("Play"));
            actionTogglePlayback->setText(tr("Play"));
        } else if (state != QtAudio::IdleState &&
                   state != QtAudio::StoppedState) {
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::resume,
                Qt::QueuedConnection
            );
            playButton->setIcon(pauseIcon);
            playButton->setToolTip(tr("Pause"));
            actionTogglePlayback->setText(tr("Pause"));
        } else if ((state == QtAudio::StoppedState ||
                    state == QtAudio::IdleState) &&
                   trackTree != nullptr &&
                   !trackTree->currentIndex().isValid()) {
            selectTrack(-1, 0);
        }
    }
}

void MainWindow::advancePlaylist(const Direction direction) {
    auto* tree = playlistView->tree(playingPlaylist);

    if (tree == nullptr) {
        return;
    }

    auto* treeModel = tree->model();
    auto* treeHeader = tree->header();

    const u16 rowCount = treeModel->rowCount();
    if (rowCount == 0) {
        return;
    }

    const QModelIndex currentIndex = tree->deselect();
    if (!currentIndex.isValid()) {
        return;
    }

    const u16 currentRow = currentIndex.row();
    u16 nextRow;

    switch (direction) {
        case Direction::Forward:
            if (currentRow == rowCount - 1) {
                switch (repeat) {
                    case RepeatMode::Playlist:
                        nextRow = 0;
                        break;
                    case RepeatMode::Off: {
                        const bool success =
                            changePlaylist(i8(playingPlaylist + 1));

                        if (!success) {
                            stopPlayback();
                            return;
                        }

                        playingPlaylist += 1;
                        tree = trackTree;
                        treeModel = trackTreeModel;
                        treeHeader = trackTreeHeader;
                        nextRow = 0;
                        break;
                    }
                    default:
                        stopPlayback();
                        return;
                }
            } else {
                nextRow = currentRow + 1;
            }
            break;
        case Direction::ForwardRandom: {
            playHistory.insert(currentRow);

            if (playHistory.size() == rowCount) {
                playHistory.clear();
            }

            do {
                nextRow = randint_range(0, rowCount - 1, rng());
            } while (playHistory.contains(nextRow));
            break;
        }
        case Direction::Backward:
        backward:
            nextRow = (currentRow == 0 ? rowCount : currentRow) - 1;
            break;
        case Direction::BackwardRandom: {
            if (playHistory.empty()) {
                goto backward;
            }

            const u16 lastPlayed = playHistory.last();
            playHistory.remove(lastPlayed);
            nextRow = lastPlayed;
            break;
        }
    }

    const QModelIndex index = treeModel->index(nextRow, 0);
    tree->setCurrentIndex(index);
    playTrack(tree, index);
}

void MainWindow::stopPlayback() {
    auto* tree = playlistView->tree(playingPlaylist);

    if (tree != nullptr) {
        tree->deselect();
    }

    peakVisualizer->reset();
    audioWorker->stop();
    trackLabel->clear();
    progressSlider->setRange(0, 0);
    progressSliderCloned->setRange(0, 0);
    updateProgressLabel(0, "00:00");
    playButton->setIcon(startIcon);
    actionTogglePlayback->setText(tr("Play"));
    setWindowTitle("RAP");

    dockCoverLabel->clear();

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT - 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(column - 1, 0))
            ->setText(1, QString());
    }
}

void MainWindow::importPlaylist(const bool createNewTab) {
    const QString filter =
        tr("XSPF/M3U/CUE Playlist (*.xspf *.m3u8 *.m3u *.cue)");
    const QUrl fileUrl = QFileDialog::getOpenFileUrl(
        this,
        tr("Open Playlist File"),
        QString(),
        filter
    );

    if (fileUrl.isEmpty()) {
        return;
    }

    const QString filePath = fileUrl.toLocalFile();

    const QFileInfo fileInfo = QFileInfo(filePath);
    const QString dirPath = fileInfo.absolutePath() + '/';
    const QString extension = fileInfo.suffix().toLower();

    QFile file = QFile(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), file.errorString());
        return;
    }

    if (extension == EXT_CUE) {
        CueInfo info = parseCUE(file, fileInfo);
        auto& metadata = info.metadata;

        QString tabName;
        tabName.reserve(isize(
            metadata[TrackProperty::Artist].size() + (sizeof(" - ") - 1) +
            metadata[TrackProperty::Title].size()
        ));
        tabName += metadata[TrackProperty::Artist];
        tabName += u" - "_qsv;
        tabName += metadata[TrackProperty::Title];

        const i8 currentIndex = playlistView->currentIndex();
        const u8 index = (createNewTab || currentIndex == -1)
                             ? playlistView->addTab(tabName)
                             : currentIndex;

        playlistView->tree(index)
            ->fillTableCUE(metadata, info.tracks, info.cueFilePath);

    } else {
        QStringList filePaths;
        filePaths.reserve(MINIMUM_TRACK_COUNT);

        const auto appendIfExists = [&](const QString& relativePath) {
            const QString localFilePath = dirPath + relativePath;

            if (QFile::exists(localFilePath)) {
                filePaths.append(localFilePath);
            }
        };

        if (extension == u"xspf"_qsv) {
            QXmlStreamReader xml = QXmlStreamReader(&file);

            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::StartElement &&
                    xml.name() == "location") {
                    appendIfExists(xml.readElementText());
                }
            }

            if (xml.hasError()) {
                QMessageBox::warning(this, tr("Error"), xml.errorString());
                return;
            }
        } else if (extension == u"m3u"_qsv || extension == u"m3u8"_qsv) {
            QTextStream input = QTextStream(&file);

            while (!input.atEnd()) {
                const QString line = input.readLine().trimmed();

                if (!line.isEmpty() && !line.startsWith('#')) {
                    appendIfExists(line);
                }
            }
        }

        if (filePaths.isEmpty()) {
            QMessageBox::information(
                this,
                tr("No Files Found"),
                tr("No valid tracks were found in the playlist.")
            );
            return;
        }

        const i8 currentIndex = playlistView->currentIndex();
        const u8 index = (createNewTab || currentIndex == -1)
                             ? playlistView->addTab(filePathInfo.fileName())
                             : currentIndex;

        playlistView->tree(index)->fillTable(filePaths);
    }
}

void MainWindow::exportPlaylist(const PlaylistFileType playlistType) {
    const i8 index = playlistView->currentIndex();
    if (index == -1) {
        return;
    }

    const auto* trackTree = playlistView->tree(index);

    QString outputPath =
        QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));

    if (outputPath.isEmpty()) {
        return;
    }

    const auto& tabLabel = playlistView->tabLabel(index);
    outputPath.reserve(1 + tabLabel.size() + 1 + 4);
    outputPath += u"/";
    outputPath += tabLabel;
    outputPath += u".";
    outputPath +=
        playlistType == PlaylistFileType::XSPF ? u"xspf"_qsv : u"m3u8"_qsv;

    if (QFile::exists(outputPath)) {
        const auto pressedButton = QMessageBox::warning(
            this,
            tr("Playlist already exists"),
            tr("Playlist already exists. Rewrite it?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (pressedButton != QMessageBox::Yes) {
            return;
        }
    }

    const u16 rowCount = trackTree->model()->rowCount();
    vector<HashMap<TrackProperty, QString>> properties;
    properties.reserve(rowCount);

    for (const u16 row : range(0, rowCount)) {
        properties.emplace_back(trackTree->rowMetadata(row));
    }

    const auto result = playlistType == PlaylistFileType::XSPF
                            ? exportXSPF(outputPath, properties)
                            : exportM3U8(outputPath, properties);

    if (!result) {
        const auto pressedButton = QMessageBox::warning(
            this,
            tr("Unable to export playlist"),
            tr("Error: %1").arg(result.error()),
            QMessageBox::Retry
        );

        if (pressedButton == QMessageBox::Retry) {
            exportPlaylist(playlistType);
        }
    }
}

static constexpr auto getXSPFTag(const TrackProperty property) -> QStringView {
    switch (property) {
        case TrackProperty::Title:
            return u"title";
        case TrackProperty::Artist:
            return u"creator";
        case TrackProperty::Album:
            return u"album";
        case TrackProperty::AlbumArtist:
            return u"albumArtist";
        case TrackProperty::Genre:
            return u"genre";
        case TrackProperty::Duration:
            return u"duration";
        case TrackProperty::TrackNumber:
            return u"trackNum";
        case TrackProperty::Comment:
            return u"annotation";
        case TrackProperty::Path:
            return u"location";
        case TrackProperty::Composer:
            return u"composer";
        case TrackProperty::Publisher:
            return u"publisher";
        case TrackProperty::Year:
            return u"year";
        case TrackProperty::BPM:
            return u"bpm";
        case TrackProperty::Language:
            return u"language";
        case TrackProperty::DiscNumber:
            return u"disc";
        case TrackProperty::Bitrate:
            return u"bitrate";
        case TrackProperty::SampleRate:
            return u"samplerate";
        case TrackProperty::Channels:
            return u"channels";
        case TrackProperty::Format:
            return u"format";
        default:
            return {};
            break;
    }
}

auto MainWindow::exportXSPF(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    QFile file = QFile(outputPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return err(file.errorString());
    }

    QTextStream output = QTextStream(&file);
    output.setEncoding(QStringConverter::Utf8);

    output << R"(<?xml version="1.0" encoding="UTF-8"?>)";
    output << '\n';
    output << R"(<playlist version="1" xmlns="http://xspf.org/ns/0/">)";
    output << '\n';
    output << "<trackList>";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        output << "<track>";
        output << '\n';

        for (const auto& [property, value] : views::drop(metadata, 1)) {
            const QStringView tag = getXSPFTag(property);

            QString content = value;

            if (property == TrackProperty::Path) {
                content =
                    QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                        .relativeFilePath(value);
            }

            output << '<' << tag << '>' << content.toHtmlEscaped() << "</"
                   << tag << '>';
            output << '\n';
        }

        output << "</track>";
        output << '\n';
    }

    output << "</trackList>";
    output << '\n';
    output << "</playlist>";
    output << '\n';

    file.close();
    return true;
}

auto MainWindow::exportM3U8(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    QFile outputFile = QFile(outputPath);

    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return err(outputFile.errorString());
    }

    QTextStream output = QTextStream(&outputFile);
    output.setEncoding(QStringConverter::Utf8);

    output << "#EXTM3U";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        const QString title = metadata[TrackProperty::Title];
        const QString artist = metadata[TrackProperty::Artist];
        const QString path = metadata[TrackProperty::Path];
        const QString durationStr = metadata[TrackProperty::Duration];

        u16 duration = 0;

        const QStringList strings = durationStr.split(':');
        const QString& minutes = strings[0];
        const QString& seconds = strings[1];

        duration += minutes.toUInt() * MINUTE_SECONDS;
        duration += seconds.toUInt();

        output << "#EXTINF:";
        output << duration << ',' << artist << " - " << title;
        output << '\n';

        output << QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                      .relativeFilePath(path);
        output << '\n';
    }

    outputFile.close();
    return true;
}

void MainWindow::adjustPlaylistView() {
    DockWidgetPosition dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? Left : Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0 ? Top : Bottom;
    }

    const i8 currentIndex = playlistView->currentIndex();

    if (currentIndex == -1) {
        return;
    }

    adjustPlaylistTabBar(dockWidgetPosition, currentIndex);

    if (!playlistView->hasBackgroundImage(currentIndex)) {
        return;
    }

    adjustPlaylistImage(dockWidgetPosition, currentIndex);
}

void MainWindow::adjustPlaylistTabBar(
    const DockWidgetPosition dockWidgetPosition,
    const u8 currentIndex
) {
    switch (dockWidgetPosition) {
        case DockWidgetPosition::Right:
            playlistTabBar->setScrollAreaWidth(dockWidget->x());
            break;
        case DockWidgetPosition::Left:
        case DockWidgetPosition::Top:
        case DockWidgetPosition::Bottom:
            playlistTabBar->setScrollAreaWidth(mainArea->width());
            break;
    }
}

void MainWindow::adjustPlaylistImage(
    const DockWidgetPosition dockWidgetPosition,
    const u8 currentIndex
) {
    const u16 width = mainArea->size().width();
    const u16 height = mainArea->size().height();

    u16 dockWidgetX = dockWidget->x();
    u16 dockWidgetY = dockWidget->y();

    if (dockWidgetX == UINT16_MAX) {
        dockWidgetX = width;
    }
    if (dockWidgetY == UINT16_MAX) {
        dockWidgetY = height;
    }

    const u16 dockWidgetWidth = dockWidget->width();
    const u16 dockWidgetHeight = dockWidget->height();

    const QWidget* pageWidget = playlistView->page(currentIndex);
    auto* leftLabel = pageWidget->findChild<QLabel*>("leftLabel");
    auto* centerLabel = pageWidget->findChild<QLabel*>("centerLabel");
    auto* rightLabel = pageWidget->findChild<QLabel*>("rightLabel");

    const u16 yPos = playlistView->y();
    const u16 centerLabelHalfWidth = centerLabel->width() / 2;
    const u16 rightLabelWidth = rightLabel->width();
    const u16 halfWidth = width / 2;

    const u16 availableWidthLeft = width - dockWidgetWidth;
    const u16 halfAvailableWidthLeft = availableWidthLeft / 2;

    const u16 availableWidthRight = dockWidgetX;
    const u16 halfAvailableWidthRight = availableWidthRight / 2;

    switch (dockWidgetPosition) {
        case DockWidgetPosition::Right:
            leftLabel->move(0, yPos);
            centerLabel->move(
                halfAvailableWidthRight - centerLabelHalfWidth,
                yPos
            );
            rightLabel->move(availableWidthRight - rightLabelWidth, yPos);
            break;
        case DockWidgetPosition::Left:
            leftLabel->move(0, yPos);
            centerLabel->move(
                halfAvailableWidthLeft - centerLabelHalfWidth,
                yPos
            );
            rightLabel->move(availableWidthLeft - rightLabelWidth, yPos);
            break;
        case DockWidgetPosition::Top:
        case DockWidgetPosition::Bottom:
            leftLabel->move(0, 0);
            centerLabel->move(halfWidth - centerLabelHalfWidth, 0);
            rightLabel->move(width - rightLabelWidth, 0);
            break;
    }
}

auto MainWindow::constructAudioFileFilter() -> QString {
    QString filter;
    filter.reserve(128);  // NOLINT

    filter += tr("Audio/Video Files (");

    for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
        filter += u"*.";
        filter += extension;
        filter += ' ';
    }

    filter.removeLast();  // Remove space
    filter += ')';
    return filter;
}