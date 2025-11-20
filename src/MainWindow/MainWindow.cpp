#include "MainWindow.hpp"

#include "AboutWindow.hpp"
#include "Aliases.hpp"
#include "Associations.hpp"
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
#include "Logger.hpp"
#include "MetadataWindow.hpp"
#include "PeakVisualizer.hpp"
#include "PlaylistView.hpp"
#include "RandInt.hpp"
#include "RapidHasher.hpp"
#include "RepeatRangeMenu.hpp"
#include "Settings.hpp"
#include "SettingsWindow.hpp"
#include "TrackProperties.hpp"
#include "TrackTree.hpp"
#include "VisualizerDialog.hpp"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QStyleFactory>
#include <QWidgetAction>
#include <QXmlStreamReader>

MainWindow::MainWindow(const QStringList& paths, QWidget* const parent) :
    QMainWindow(parent) {
    dockWidget->dockCoverLabel = dockCoverLabel;
    dockWidget->dockMetadataTree = dockMetadataTree;

    connect(
        equalizerButton,
        &QPushButton::pressed,
        this,
        &MainWindow::toggleEqualizerMenu
    );

    connect(actionForward, &QAction::triggered, this, [&] -> void {
        advancePlaylist(forwardDirection);
    });

    connect(actionBackward, &QAction::triggered, this, [&] -> void {
        advancePlaylist(backwardDirection);
    });

    connect(actionRepeat, &QAction::triggered, this, &MainWindow::toggleRepeat);

    connect(actionTogglePlayback, &QAction::triggered, this, [&] -> void {
        togglePlayback();
    });

    connect(actionStop, &QAction::triggered, this, &MainWindow::stopPlayback);

    connect(actionRandom, &QAction::triggered, this, &MainWindow::toggleRandom);

    connect(actionVisualizer, &QAction::triggered, this, [&] -> void {
#ifdef PROJECTM
        if (visualizerDialog != nullptr) {
            return;
        }

        visualizerDialog = new VisualizerDialog(settings);

        connect(visualizerDialog, &VisualizerDialog::ready, this, [&] -> void {
            visualizerDialog->setChannels(audioWorker->channels());
            visualizerDialog->show();

            audioWorker->toggleVisualizer(true);

            connect(
                audioWorker,
                &AudioWorker::audioProperties,
                visualizerDialog,
                [this](const u32 /* sampleRate */, const AudioChannels channels)
                    -> void { visualizerDialog->setChannels(channels); }
            );

            connect(
                audioWorker,
                &AudioWorker::processedSamples,
                visualizerDialog,
                [&] -> void {
                visualizerDialog->addSamples(visualizerBuffer.data());
            }
            );
        });

        connect(
            audioWorker,
            &AudioWorker::streamEnded,
            visualizerDialog,
            &VisualizerDialog::clear
        );

        connect(
            visualizerDialog,
            &VisualizerDialog::rejected,
            this,
            [&] -> void {
            delete visualizerDialog;
            visualizerDialog = nullptr;
            audioWorker->toggleVisualizer(false);
        },
            Qt::SingleShotConnection
        );
#else
        QMessageBox::information(
            this,
            tr("Visualizer disabled"),
            tr("The program was compiled without projectM visualizer library.")
        );
#endif
    });

    connect(
        actionSettings,
        &QAction::triggered,
        this,
        &MainWindow::showSettingsWindow
    );

    connect(actionExit, &QAction::triggered, this, &MainWindow::exit);

    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &MainWindow::onTrayIconActivated
    );

    connect(actionOpenFolder, &QAction::triggered, this, [&] -> void {
        addEntry(true, true);
    });

    connect(actionOpenFile, &QAction::triggered, this, [&] -> void {
        addEntry(true, false);
    });

    connect(actionAddFolder, &QAction::triggered, this, [&] -> void {
        addEntry(false, true);
    });

    connect(actionAddFile, &QAction::triggered, this, [&] -> void {
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
        [&](const u16 value) -> void { updateProgressLabel(value); }
    );

    connect(
        progressSliderTray,
        &CustomSlider::sliderReleased,
        this,
        &MainWindow::updatePlaybackPosition
    );

    connect(
        progressSliderTray,
        &CustomSlider::valueChanged,
        this,
        [&](const u16 value) -> void { updateProgressLabel(value); }
    );

    connect(
        progressSlider,
        &CustomSlider::valueChanged,
        progressSliderTray,
        &CustomSlider::setValue
    );

    connect(
        progressSliderTray,
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
        volumeSliderTray,
        &CustomSlider::setValue
    );

    connect(
        volumeSliderTray,
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
        actionDocumentation,
        &QAction::triggered,
        this,
        &MainWindow::showHelpWindow
    );

    connect(actionOpenPlaylist, &QAction::triggered, this, [&] -> void {
        importPlaylist(true);
    });

    connect(actionAddPlaylist, &QAction::triggered, this, [&] -> void {
        importPlaylist(false);
    });

    connect(actionExportM3U8Playlist, &QAction::triggered, this, [&] -> void {
        exportPlaylist(PlaylistFileType::M3U);
    });

    connect(actionExportXSPFPlaylist, &QAction::triggered, this, [&] -> void {
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
        [&](const TabRemoveMode mode,
            const u8 startIndex,
            const u8 count) -> void {
        if (playingPlaylist == 0 || playingPlaylist < startIndex) {
            return;
        }

        switch (mode) {
            case TabRemoveMode::Single:
                playingPlaylist -= 1;
                break;
            case TabRemoveMode::ToLeft:
            case TabRemoveMode::Other:
                playingPlaylist = i8(playingPlaylist - count);
                break;
            default:
                break;
        }
    }
    );

    connect(actionRussian, &QAction::triggered, this, [&] -> void {
        retranslate(QLocale::Russian);
    });

    connect(actionEnglish, &QAction::triggered, this, [&] -> void {
        retranslate(QLocale::English);
    });

    connect(
        dockWidget,
        &DockWidget::repositioned,
        this,
        &MainWindow::moveDockWidget
    );

    connect(server, &QLocalServer::newConnection, this, [&] -> void {
        QLocalSocket* const clientConnection = server->nextPendingConnection();

        connect(
            clientConnection,
            &QLocalSocket::readyRead,
            [this, clientConnection] -> void {
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

    connect(
        ui->controlContainer,
        &QWidget::customContextMenuRequested,
        this,
        [&] -> void {
        auto* const menu = new QMenu(this);

        const bool visualizerShown = !peakVisualizer->isHidden();
        QAction* const visualizerAction = menu->addAction(tr("Visualizer"));
        visualizerAction->setCheckable(true);
        visualizerAction->setChecked(visualizerShown);

        const QAction* const selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == visualizerAction) {
            peakVisualizer->setHidden(visualizerShown);
            audioWorker->togglePeakVisualizer(!visualizerShown);
        }

        if (peakVisualizer->isHidden()) {
            peakVisualizer->stop();
        } else if (audioWorker->state() == ma_device_state_started) {
            peakVisualizer->start();
        }
    }
    );

    connect(
        repeatButton,
        &IconTextButton::customContextMenuRequested,
        this,
        [&](const QPoint& pos) -> void {
        if (repeat != RepeatMode::Track) {
            return;
        }

        if (repeatRangeMenu->isHidden()) {
            repeatRangeMenu->move(
                repeatButton->mapToGlobal(QPoint{ 0, repeatButton->height() })
            );

            repeatRangeMenu->show();
        }
    }
    );

#ifdef Q_OS_LINUX
    // On Linux, server cannot listen to the given name if it's already exists.
    // Since everything is file in Unix systems, server is persistent across
    // runs.
    QLocalServer::removeServer(u"revolutionary-audio-player-server"_s);
#endif

    server->listen(u"revolutionary-audio-player-server"_s);

    progressLabelTray->setText(trackDuration);
    progressSliderTray->setRange(0, 0);
    volumeSliderTray->setRange(0, MAX_VOLUME);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

    playButton->setAction(actionTogglePlayback);
    stopButton->setAction(actionStop);
    backwardButton->setAction(actionBackward);
    forwardButton->setAction(actionForward);
    repeatButton->setAction(actionRepeat);
    randomButton->setAction(actionRandom);

    searchTrackInput->hide();
    searchTrackInput->setTextMargins(SEARCH_INPUT_TEXT_MARGINS);
    searchTrackInput->setMinimumWidth(SEARCH_INPUT_MIN_WIDTH);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    setupTrayIcon();
    initializeSettings();

    playlistView->setSettings(settings);
    audioWorker = new AudioWorker(
        settings,
        peakVisualizerBuffer.data(),
        visualizerBuffer.data()
    );
    equalizerMenu = new EqualizerMenu(settings, this);

    audioWorker->togglePeakVisualizer(true);

    connect(
        equalizerMenu,
        &EqualizerMenu::gainChanged,
        audioWorker,
        &AudioWorker::changeGain
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

    loadSettings();

    spectrumVisualizer =
        new SpectrumVisualizer(spectrumVisualizerBuffer.data(), settings, this);

    ui->controlLayout->insertWidget(
        ui->controlLayout->indexOf(equalizerButton) + 1,
        spectrumVisualizer
    );

    connect(
        audioWorker,
        &AudioWorker::audioProperties,
        peakVisualizer,
        &PeakVisualizer::setAudioProperties
    );

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
        const auto firstPathInfo = QFileInfo(args[0]);
        QString tabLabel;

        if (settings->playlist.playlistNaming ==
            PlaylistNaming::DirectoryName) {
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
            [&](const bool startPlaying) -> void {
            if (startPlaying) {
                togglePlayback();
            }
        },
            Qt::SingleShotConnection
        );
    }
}

void MainWindow::closeEvent(QCloseEvent* const event) {
    hide();
    event->ignore();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* const event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* const event) {
    const QString data = event->mimeData()->text();

    if (data == "spectrumvisualizer") {
        spectrumVisualizer->setParent(ui->controlContainer);
        ui->controlLayout->insertWidget(7, spectrumVisualizer);
        spectrumVisualizer->show();
        return;
    }

    const QPoint dropPos = event->position().toPoint();
    TrackTree* tree = trackTree;

    const QList<QUrl> urls = event->mimeData()->urls();

    QStringList filePaths;
    filePaths.reserve(urls.size());

    QStringList dirPaths;
    dirPaths.reserve(urls.size());

    QStringList playlistPaths;
    playlistPaths.reserve(urls.size());

    for (const QUrl& url : urls) {
        const QString path = url.toLocalFile();

        if (url.isEmpty()) {
            continue;
        }

        const auto info = QFileInfo(path);

        if (info.isDir()) {
            dirPaths.append(path);
            continue;
        }

        const QString extension = info.suffix().toLower();

        if (ranges::contains(ALLOWED_PLAYLIST_EXTENSIONS, extension)) {
            playlistPaths.append(path);
            continue;
        }

        if (ranges::contains(ALLOWED_PLAYABLE_EXTENSIONS, extension)) {
            filePaths.append(path);
        }
    }

    const bool outsideTree =
        tree == nullptr ||
        !QRect(tree->mapToGlobal(QPoint(0, 0)), tree->size()).contains(dropPos);

    if (outsideTree) {
        for (const QString& dirPath : dirPaths) {
            const auto info = QFileInfo(dirPath);
            auto* newTree =
                playlistView->tree(playlistView->addTab(info.fileName()));
            newTree->fillTable({ dirPath });
        }

        for (const QString& playlistPath : playlistPaths) {
            importPlaylist(true, playlistPath);
        }

        if (!filePaths.isEmpty()) {
            const auto info = QFileInfo(filePaths[0]);

            tree =
                playlistView->tree(playlistView->addTab(info.dir().dirName()));

            tree->fillTable(filePaths);
        }
    } else {
        tree->fillTable(dirPaths);

        if (!filePaths.isEmpty()) {
            tree->fillTable(filePaths);
        }

        for (const QString& playlistPath : playlistPaths) {
            importPlaylist(false, playlistPath);
        }
    }
}

void MainWindow::updatePlaybackPosition() {
    u16 second = progressSlider->value();

    if (CUEOffset != UINT16_MAX) {
        second += CUEOffset;
    }

    audioWorker->seekSecond(second);
    updateProgressLabel(second);
}

void MainWindow::loadPlaylists() {
    auto playlistsFile = QFile(
        settings->core.playlistsDir + '/' +
        PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    );

    if (!playlistsFile.open(QFile::ReadOnly | QFile::Text)) {
        LOG_WARN(playlistsFile.errorString());
        return;
    }

    const QJsonArray playlistsArray =
        QJsonDocument::fromJson(playlistsFile.readAll()).array();

    for (const auto& playlistObject : playlistsArray) {
        const PlaylistObject playlist =
            PlaylistObject(playlistObject.toObject());

        const u8 index =
            playlistView->addTab(playlist.label, playlist.defaultColumns);

        TrackTree* const tree = playlistView->tree(index);
        tree->fillTable(playlist.tracks, playlist.cueOffsets);

        if (!playlist.order.empty()) {
            connect(
                tree,
                &TrackTree::fillingFinished,
                this,
                [=](const bool /* startPlaying */) -> void {
                for (const u16 track : range(0, tree->model()->rowCount())) {
                    tree->model()
                        ->item(track, i32(TrackProperty::Order))
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

            if (ranges::contains(ALLOWED_AUDIO_EXTENSIONS, extension)) {
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

            QTimer::singleShot(SECOND_MS, [=, this] -> void {
                playlistView->setBackgroundImage(index, image, imagePath);
            });
        }
    }
}

auto MainWindow::savePlaylists() -> result<bool, QString> {
    QFile playlistsFile = QFile(
        settings->core.playlistsDir + '/' +
        PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    );

    if (!playlistsFile.open(QFile::WriteOnly | QFile::Text)) {
        return err(playlistsFile.errorString());
    }

    QJsonArray playlistsArray;
    const u8 tabCount = playlistView->tabCount();

    for (const u8 tab : range(0, tabCount)) {
        const TrackTree* const tree = playlistView->tree(tab);
        const TrackTreeModel* const model = tree->model();
        const QString& tabColor = playlistView->tabColor(tab);

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

        for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
            if (tree->isColumnHidden(column)) {
                playlistObject.defaultColumns[column - 1] = TrackProperty::Play;
                continue;
            }

            playlistObject.defaultColumns[column - 1] = TrackProperty(
                model->headerData(column, Qt::Horizontal, PROPERTY_ROLE)
                    .toUInt()
            );
        }

        playlistsArray.append(playlistObject.stringify());
    }

    playlistsFile.write(
        QJsonDocument(playlistsArray).toJson(QJsonDocument::Compact)
    );
    return true;
}

auto MainWindow::saveSettings() -> result<bool, QString> {
    QFile settingsFile = QFile(
        settings->core.settingsDir + '/' +
        SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    );

    if (!settingsFile.open(QFile::WriteOnly | QFile::Text)) {
        return err(settingsFile.errorString());
    }

    settings->core.currentTab = playlistView->currentIndex();
    settings->core.volume = volumeSlider->value();

    equalizerMenu->saveSettings();

    DockWidgetPosition dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidgetPosition::Left
                                                  : DockWidgetPosition::Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidgetPosition::Top
                                                  : DockWidgetPosition::Bottom;
    }

    settings->dockWidget.position = dockWidgetPosition;
    settings->dockWidget.size = dockWidget->width();
    settings->dockWidget.imageSize = dockCoverLabel->height();

    settings->save(settingsFile);

    QFile rapPathsFile = QFile(
        QApplication::applicationDirPath() + '/' +
        PATHS_FILE_NAME
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    );

    if (rapPathsFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        rapPathsFile.write(settings->core.settingsDir.toUtf8());
        rapPathsFile.write("\n");
        rapPathsFile.write(settings->core.playlistsDir.toUtf8());
    }

    return savePlaylists();
}

void MainWindow::retranslate(const QLocale::Language language) {
    QApplication::removeTranslator(translator);
    delete translator;

    translator = new QTranslator(this);
    const bool success = translator->load(
        QLocale(language),
        u"rap"_s,
        u"."_s,
        QApplication::applicationDirPath() + u"/translations"_qssv
    );

    QApplication::installTranslator(translator);

    ui->retranslateUi(this);

    const u8 tabCount = playlistView->tabCount();
    const QStringList propertyLabelMap = trackPropertiesLabels();

    for (const u8 tab : range(0, tabCount)) {
        TrackTree* const tree = playlistView->tree(tab);
        TrackTreeModel* const model = tree->model();

        for (const TrackProperty property : DEFAULT_COLUMN_PROPERTIES) {
            const u8 column = u8(property);

            model->setHeaderData(
                column,
                Qt::Horizontal,
                propertyLabelMap[column]
            );
            tree->resizeColumnToContents(column);
        }
    }

    equalizerMenu->retranslate();
    searchTrackInput->setPlaceholderText(tr("Track name/property"));
}

void MainWindow::initializeSettings() {
    QFile rapPathsFile = QFile(
        QApplication::applicationDirPath() + '/' +
        PATHS_FILE_NAME
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    );
    QFile* settingsFile;
    QString settingsFilePath;
    QString playlistsFilePath;

    if (!rapPathsFile.open(QFile::ReadOnly | QFile::Text)) {
        LOG_WARN(rapPathsFile.errorString());

        settingsFilePath = QApplication::applicationDirPath() + '/' +
                           SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
                               .toString()
#endif
            ;
        playlistsFilePath = QApplication::applicationDirPath() + '/' +
                            PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                .toString()
#endif
            ;

        settingsFile = new QFile(settingsFilePath);

    } else {
        QString settingsDir = rapPathsFile.readLine();
        QString playlistsDir = rapPathsFile.readLine();

        // Remove "\n", playlistsDir doesn't required that because of EOF
        settingsDir.chop(1);

        settingsFilePath = settingsDir + '/' +
                           SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
                               .toString()
#endif
            ;
        playlistsFilePath = playlistsDir + '/' +
                            PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                .toString()
#endif
            ;

        settingsFile = new QFile(settingsFilePath);
    }

    if (!settingsFile->open(QFile::ReadOnly | QFile::Text)) {
        LOG_WARN(settingsFile->fileName() + settingsFile->errorString());

        settings = make_shared<Settings>();
        delete settingsFile;

        return;
    }

    const QByteArray jsonData = settingsFile->readAll();

    QJsonParseError jsonError;
    const QJsonObject settingsObject =
        QJsonDocument::fromJson(jsonData, &jsonError).object();

    if (jsonError.error != QJsonParseError::NoError) {
        LOG_WARN(jsonError.errorString());
    } else {
        settings = make_shared<Settings>(settingsObject);
    }

    settings->core.settingsDir =
        settingsFilePath.slice(0, settingsFilePath.lastIndexOf('/'));
    settings->core.playlistsDir =
        playlistsFilePath.slice(0, playlistsFilePath.lastIndexOf('/'));

    delete settingsFile;
}

void MainWindow::loadSettings() {
    volumeSlider->setValue(settings->core.volume);
    volumeSliderTray->setValue(settings->core.volume);

    updateVolume(settings->core.volume);

    settings->core.defaultStyle = QApplication::style()->name();
    QApplication::setStyle(settings->core.applicationStyle);

    SettingsWindow::setTheme(settings->core.applicationTheme);

    loadPlaylists();
    playlistView->setCurrentIndex(settings->core.currentTab);
    equalizerMenu->loadSettings();

    // Directly update system associations, in case if binary was moved
    // somewhere
    if (shellEntryExists()) {
        settings->shell.contextMenuEntryEnabled = true;
        createContextMenuEntry();
    }

    const Associations associations = getExistingAssociations();

    if (associations != Associations::None) {
        settings->shell.enabledAssociations = associations;

        updateFileAssociations(associations);
    }

    moveDockWidget(settings->dockWidget.position);
    mainArea->setSizes({ QWIDGETSIZE_MAX, settings->dockWidget.size });

    retranslate(settings->core.language);
}

void MainWindow::moveDockWidget(const DockWidgetPosition dockWidgetPosition) {
    Qt::Orientation targetMainOrientation;
    Qt::Orientation targetDockOrientation;
    u8 targetIndex;

    switch (dockWidgetPosition) {
        case DockWidgetPosition::Left:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 0;
            break;
        case DockWidgetPosition::Right:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 1;
            break;
        case DockWidgetPosition::Top:
            targetMainOrientation = Qt::Vertical;
            targetDockOrientation = Qt::Horizontal;
            targetIndex = 0;
            break;
        case DockWidgetPosition::Bottom:
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

            auto* const menu = new QMenu(this);

            const QAction* const removeAction =
                menu->addAction(tr("Remove Track"));

            QModelIndexList selectedRows =
                trackTree->selectionModel()->selectedRows();
            const QAction* removeSelectionAction = nullptr;

            if (selectedRows.size() > 2) {
                removeSelectionAction = menu->addAction(tr("Remove Selection"));
            }

            const QAction* const clearAction =
                menu->addAction(tr("Clear All Tracks"));
            const QAction* const metadataAction =
                menu->addAction(tr("Show Track Metadata"));
            const QAction* const coverAction =
                menu->addAction(tr("Show Cover"));
            const QAction* const setPlaylistBackground =
                menu->addAction(tr("Set Playlist Background"));

            const QAction* removePlaylistBackground = nullptr;

            if (playlistView->hasBackgroundImage(
                    playlistView->currentIndex()
                )) {
                removePlaylistBackground =
                    menu->addAction(tr("Remove Playlist Background"));
            }

            const QAction* const changeOpacityAction =
                menu->addAction(tr("Change Opacity"));

            const QAction* const selectedAction = menu->exec(QCursor::pos());
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
                auto* const metadataWindow = new MetadataWindow(
                    trackTree->rowMetadata(index.row()),
                    this
                );
                metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                metadataWindow->show();
            } else if (selectedAction == coverAction) {
                const HashMap<TrackProperty, QString> metadata =
                    trackTree->rowMetadata(index.row());

                auto* const coverWindow = new CoverWindow(
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
            } else if (selectedAction == changeOpacityAction) {
                auto* const opacityInput = new InputPopup(
                    QString::number(trackTree->opacity(), 'f', 2),
                    QCursor::pos(),
                    this
                );

                auto* const doubleValidator =
                    new QDoubleValidator(0.0, 1.0, 2, opacityInput);

                opacityInput->setValidator(doubleValidator);
                opacityInput->show();

                connect(
                    opacityInput,
                    &InputPopup::finished,
                    this,
                    [this](const QString& text) -> void {
                    trackTree->setOpacity(QLocale::system().toFloat(text));
                },
                    Qt::SingleShotConnection
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
        TrackProperty property = TrackProperty::Title;

        if (colonPos != -1) {
            prefix = input.sliced(0, colonPos);
            value = input.sliced(colonPos + 1);
            hasPrefix = true;

            const auto iter = ranges::find_if(
                SEARCH_PROPERTIES,
                [&](const QStringView property) -> bool {
                return prefix == property;
            }
            );

            if (iter != SEARCH_PROPERTIES.end()) {
                const usize index =
                    ranges::distance(SEARCH_PROPERTIES.begin(), iter);
                property = TrackProperty(index);
            } else {
                searchTrackInput->clear();
                searchTrackInput->setPlaceholderText(tr("Incorrect property!"));
            }
        }

        for (const u16 row : range(0, trackTreeModel->rowCount())) {
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

        if (searchMatches.isEmpty()) {
            searchMatches.clear();
            searchMatches.reserve(MINIMUM_MATCHES_NUMBER);
            searchMatchesPosition = 0;
            return;
        }
    }

    if (searchMatches.isEmpty()) {
        return;
    }

    if (searchMatchesPosition >= searchMatches.size()) {
        searchMatchesPosition = 0;
    }

    trackTree->selectionModel()->clearSelection();

    const QModelIndex& match = searchMatches[searchMatchesPosition++];
    trackTree->scrollTo(match, QTreeView::PositionAtCenter);
    trackTree->selectionModel()->select(
        match,
        QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::showSearchInput() {
    if (trackTree == nullptr) {
        return;
    }

    searchTrackInput->move(trackTree->mapToGlobal(QPoint(
        trackTree->width() - searchTrackInput->width() -
            SEARCH_INPUT_POSITION_PADDING,
        0 + SEARCH_INPUT_POSITION_PADDING
    )));
    searchTrackInput->raise();
    searchTrackInput->show();
    searchTrackInput->setFocus();
}

void MainWindow::onAudioProgressUpdated(u16 seconds) {
    const bool draggingSlider =
        progressSlider->isSliderDown() || progressSliderTray->isSliderDown();

    if (!draggingSlider) {
        if (repeat == RepeatMode::Track) {
            u16 startSecond = repeatRangeMenu->startSecond();
            u16 endSecond = repeatRangeMenu->endSecond();
            auto skipSections = repeatRangeMenu->skipSections();

            if (CUEOffset != UINT16_MAX) {
                startSecond += CUEOffset;
                endSecond += CUEOffset;

                for (auto& section : skipSections) {
                    section.start += CUEOffset;
                    section.end += CUEOffset;
                }
            }

            for (const auto& section : skipSections) {
                if (seconds >= section.start && seconds <= section.end) {
                    audioWorker->seekSecond(section.end);
                }
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

            if (seconds == secsDuration) {
                playNext();
            }
        }

        progressSlider->setValue(seconds);
    }
}

void MainWindow::playNext() {
    switch (repeat) {
        case RepeatMode::Track:
            audioWorker->seekSecond(0);
            break;
        default:
            advancePlaylist(forwardDirection);
            break;
    }
}

void MainWindow::showSettingsWindow() {
    auto* const settingsWindow = new SettingsWindow(settings, this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->show();

    connect(
        settingsWindow,
        &SettingsWindow::audioDeviceChanged,
        audioWorker,
        &AudioWorker::changeAudioDevice
    );
}

void MainWindow::showHelpWindow() {
    auto* const helpWindow = new HelpWindow(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->show();
}

void MainWindow::addEntry(const bool createNewTab, const bool isFolder) {
    const QString searchDirectory =
        settings->core.lastDir.isEmpty() ||
                !QFile::exists(settings->core.lastDir)
            ? QDir::rootPath()
            : settings->core.lastDir;
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

        settings->core.lastDir = path;
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

    TrackTree* tree = trackTree;

    if (createNewTab || tree == nullptr) {
        const u8 index = playlistView->addTab(
            settings->playlist.playlistNaming == PlaylistNaming::DirectoryName
                ? QFileInfo(path).fileName()
                : QString(),
            settings->playlist.defaultColumns
        );
        tree = playlistView->tree(index);
    }

    tree->fillTable({ path });
}

void MainWindow::showAboutWindow() {
    auto* const aboutWindow = new AboutWindow(this);
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
}

inline void
MainWindow::updateProgressLabel(const u16 second, const QString& duration) {
    QString newTrackDuration;
    newTrackDuration.reserve(5 + 1 + 5);
    newTrackDuration += secsToMins(second);
    newTrackDuration += '/';
    newTrackDuration +=
        duration.isEmpty() ? trackDuration.slice(trackDuration.indexOf('/') + 1)
                           : duration;
    trackDuration = newTrackDuration;

    progressLabel->setText(trackDuration);
    progressLabelTray->setText(trackDuration);
}

void MainWindow::updateVolume(const u8 value) {
    const QString formattedVolume = QString::number(value) + '%';

    volumeLabel->setText(formattedVolume);
    volumeLabelTray->setText(formattedVolume);

    audioWorker->setVolume(f32(value) / MAX_VOLUME);
}

void MainWindow::cancelSearchInput() {
    searchTrackInput->clear();
    searchTrackInput->hide();
}

void MainWindow::toggleEqualizerMenu() {
    if (equalizerMenu->isHidden()) {
        equalizerMenu->show();

        equalizerMenu->move(
            equalizerButton->mapToGlobal(QPoint(0, equalizerButton->height()))
        );
    }
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
        QMessageBox messageBox = QMessageBox(this);
        messageBox.setWindowTitle(tr("Save Failed!"));
        messageBox.setText(saveResult.error());
        messageBox.setIcon(QMessageBox::Warning);

        QPushButton* const tryAgainButton =
            messageBox.addButton(tr("Try Again"), QMessageBox::AcceptRole);
        QPushButton* const cancelButton =
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
    trayIcon->setIcon(QIcon(
        QApplication::applicationDirPath() + '/' +
        PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
            .toString()
#endif
    ));
    trayIcon->show();

#ifdef Q_OS_WINDOWS
    auto* volumeSliderAction = new QWidgetAction(trayIconMenu);

    auto* const volumeSliderContainer = new QWidget(trayIconMenu);
    auto* const volumeSliderLayout = new QHBoxLayout(volumeSliderContainer);

    volumeSliderLayout->addWidget(volumeSliderTray);
    volumeSliderLayout->addWidget(volumeLabelTray);

    volumeSliderAction->setDefaultWidget(volumeSliderContainer);

    auto* progressSliderAction = new QWidgetAction(trayIconMenu);

    auto* const progressSliderContainer = new QWidget(trayIconMenu);
    auto* const progressSliderLayout = new QHBoxLayout(progressSliderContainer);

    progressSliderLayout->addWidget(progressSliderTray);
    progressSliderLayout->addWidget(progressLabelTray);

    progressSliderAction->setDefaultWidget(progressSliderContainer);
#endif

    trayIconMenu->addActions(
        {
#ifdef Q_OS_WINDOWS
            volumeSliderAction,
            progressSliderAction,
#endif
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
        TrackTree* const tree = playlistView->tree(playingPlaylist);

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
    artistAndTrack += u" - "_qsv;
    artistAndTrack += metadata[TrackProperty::Title];
    trackLabel->setText(artistAndTrack);

    QString windowTitle;
    windowTitle.reserve(isize(sizeof("RAP: ") - 1) + artistAndTrack.size());
    windowTitle += u"RAP: "_qsv;
    windowTitle += artistAndTrack;
    setWindowTitle(windowTitle);

    for (const u8 property : range(1, TRACK_PROPERTY_COUNT - 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(property - 1, 0))
            ->setText(1, metadata[TrackProperty(property)]);
    }

    QPixmap cover;
    QString path;

    if (settings->playlist.prioritizeExternalCover) {
        const QDir fileDirectory = metadata[TrackProperty::Path].sliced(
            0,
            metadata[TrackProperty::Path].lastIndexOf('/')
        );

        const QStringList entries =
            fileDirectory.entryList(QDir::Files | QDir::NoDotAndDotDot);

        for (const QString& entry : entries) {
            const QString extension =
                entry.sliced(entry.lastIndexOf('.') + 1).toLower();

            if (!ranges::contains(ALLOWED_IMAGE_EXTENSIONS, extension)) {
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

    if (!settings->playlist.prioritizeExternalCover || path.isEmpty()) {
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

    if (settings->playlist.autoSetBackground) {
        const QImage image = cover.toImage();
        playlistView->setBackgroundImage(playingPlaylist, image, path);
    }

    const u16 seconds = timeToSecs(metadata[TrackProperty::Duration]);
    repeatRangeMenu->setDuration(seconds);
    progressSlider->setRange(0, seconds);
    progressSliderTray->setRange(0, seconds);
    progressSlider->setValue(0);
    progressSliderTray->setValue(0);
    updateProgressLabel(0, metadata[TrackProperty::Duration]);

    dockCoverLabel->clear();
    dockCoverLabel->setPixmap(cover);
}

void MainWindow::togglePlayback(const QString& path, const u16 startSecond) {
    if (!path.isEmpty()) {
        if (currentTrack == path) {
            audioWorker->seekSecond(startSecond);
            return;
        }

        audioWorker->start(path, startSecond);
        currentTrack = path;
        playButton->setIcon(pauseIcon);
        playButton->setToolTip(tr("Pause"));
        actionTogglePlayback->setText(tr("Pause"));

        if (!peakVisualizer->isHidden()) {
            peakVisualizer->start();
        }
    } else {
        const ma_device_state state = audioWorker->state();

        if (state == ma_device_state_started) {
            audioWorker->pause();
            playButton->setIcon(startIcon);
            playButton->setToolTip(tr("Play"));
            actionTogglePlayback->setText(tr("Play"));

            if (visualizerDialog != nullptr) {
                visualizerDialog->clear();
            }

            peakVisualizer->stop();
        } else if (state == ma_device_state_stopped) {
            if (trackTree != nullptr && !trackTree->currentIndex().isValid()) {
                selectTrack(-1, 0);
            } else {
                audioWorker->resume();
            }

            playButton->setIcon(pauseIcon);
            playButton->setToolTip(tr("Pause"));
            actionTogglePlayback->setText(tr("Pause"));

            if (!peakVisualizer->isHidden()) {
                peakVisualizer->start();
            }
        } else if (state == ma_device_state_uninitialized &&
                   trackTree != nullptr &&
                   !trackTree->currentIndex().isValid()) {
            selectTrack(-1, 0);
            if (!peakVisualizer->isHidden()) {
                peakVisualizer->start();
            }
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
    TrackTree* const tree = playlistView->tree(playingPlaylist);

    if (tree != nullptr) {
        tree->deselect();
    }

    currentTrack = QString();
    peakVisualizer->reset();
    audioWorker->stop();
    trackLabel->clear();
    progressSlider->setRange(0, 0);
    progressSliderTray->setRange(0, 0);
    updateProgressLabel(0, ZERO_DURATION);
    playButton->setIcon(startIcon);
    actionTogglePlayback->setText(tr("Play"));
    setWindowTitle(u"RAP"_s);

    dockCoverLabel->clear();

    for (const u8 column : range(1, TRACK_PROPERTY_COUNT - 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(column - 1, 0))
            ->setText(1, QString());
    }
}

void MainWindow::importPlaylist(const bool createNewTab, QString filePath) {
    if (filePath.isEmpty()) {
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

        filePath = fileUrl.toLocalFile();
    }

    const auto fileInfo = QFileInfo(filePath);
    const QString dirPath = fileInfo.absolutePath() + '/';
    const QString extension = fileInfo.suffix().toLower();

    auto file = QFile(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(
            this,
            tr("Error when importing playlist"),
            file.errorString()
        );
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

        const auto appendIfExists = [&](const QString& relativePath) -> void {
            const QString localFilePath = dirPath + relativePath;

            if (QFile::exists(localFilePath)) {
                filePaths.append(localFilePath);
            }
        };

        if (extension == EXT_XSPF) {
            QXmlStreamReader xml = QXmlStreamReader(&file);

            while (!xml.atEnd()) {
                if (xml.readNext() == QXmlStreamReader::StartElement &&
                    xml.name() == u"location"_qsv) {
                    appendIfExists(xml.readElementText());
                }
            }

            if (xml.hasError()) {
                QMessageBox::warning(this, tr("Error"), xml.errorString());
                return;
            }
        } else if (extension == EXT_M3U || extension == EXT_M3U8) {
            auto input = QTextStream(&file);

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
                             ? playlistView->addTab(fileInfo.fileName())
                             : currentIndex;

        playlistView->tree(index)->fillTable(filePaths);
    }
}

void MainWindow::exportPlaylist(const PlaylistFileType playlistType) {
    const i8 index = playlistView->currentIndex();
    if (index == -1) {
        return;
    }

    const TrackTree* const trackTree = playlistView->tree(index);

    QString outputPath =
        QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));

    if (outputPath.isEmpty()) {
        return;
    }

    const auto& tabLabel = playlistView->tabLabel(index);
    outputPath.reserve(1 + tabLabel.size() + 1 + 4);
    outputPath += '/';
    outputPath += tabLabel;
    outputPath += '.';
    outputPath += playlistType == PlaylistFileType::XSPF ? EXT_XSPF : EXT_M3U8;

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
            tr("Error: ") + result.error(),
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
            return u"title"_qsv;
        case TrackProperty::Artist:
            return u"creator"_qsv;
        case TrackProperty::Album:
            return u"album"_qsv;
        case TrackProperty::AlbumArtist:
            return u"albumArtist"_qsv;
        case TrackProperty::Genre:
            return u"genre"_qsv;
        case TrackProperty::Duration:
            return u"duration"_qsv;
        case TrackProperty::TrackNumber:
            return u"trackNum"_qsv;
        case TrackProperty::Comment:
            return u"annotation"_qsv;
        case TrackProperty::Path:
            return u"location"_qsv;
        case TrackProperty::Composer:
            return u"composer"_qsv;
        case TrackProperty::Publisher:
            return u"publisher"_qsv;
        case TrackProperty::Year:
            return u"year"_qsv;
        case TrackProperty::BPM:
            return u"bpm"_qsv;
        case TrackProperty::Language:
            return u"language"_qsv;
        case TrackProperty::DiscNumber:
            return u"disc"_qsv;
        case TrackProperty::Bitrate:
            return u"bitrate"_qsv;
        case TrackProperty::SampleRate:
            return u"samplerate"_qsv;
        case TrackProperty::Channels:
            return u"channels"_qsv;
        case TrackProperty::Format:
            return u"format"_qsv;
        default:
            return {};
            break;
    }
}

auto MainWindow::exportXSPF(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    auto file = QFile(outputPath);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return err(file.errorString());
    }

    auto output = QTextStream(&file);
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

    return true;
}

auto MainWindow::exportM3U8(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    auto outputFile = QFile(outputPath);

    if (!outputFile.open(QFile::WriteOnly | QFile::Text)) {
        return err(outputFile.errorString());
    }

    auto output = QTextStream(&outputFile);
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

    return true;
}

void MainWindow::adjustPlaylistView() {
    DockWidgetPosition dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidgetPosition::Left
                                                  : DockWidgetPosition::Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidgetPosition::Top
                                                  : DockWidgetPosition::Bottom;
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
        case DockWidgetPosition::Right: {
            const i32 dockWidgetX = dockWidget->x();

            if (dockWidgetX == -1) {
                playlistTabBar->setScrollAreaWidth(mainArea->width());
            } else {
                playlistTabBar->setScrollAreaWidth(dockWidgetX);
            }
            break;
        }
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
    auto* leftLabel = pageWidget->findChild<QLabel*>(u"leftLabel"_qsv);
    auto* centerLabel = pageWidget->findChild<QLabel*>(u"centerLabel"_qsv);
    auto* rightLabel = pageWidget->findChild<QLabel*>(u"rightLabel"_qsv);

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

    for (const QStringView extension : ALLOWED_PLAYABLE_EXTENSIONS) {
        filter += u"*."_qsv;
        filter += extension;
        filter += ' ';
    }

    filter.removeLast();  // Remove space
    filter += ')';
    return filter;
}