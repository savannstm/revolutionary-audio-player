#include "MainWindow.hpp"

#include "AboutWindow.hpp"
#include "Aliases.hpp"
#include "Associations.hpp"
#include "AudioWorker.hpp"
#include "AutoUpdater.hpp"
#include "Constants.hpp"
#include "CoverWindow.hpp"
#include "CustomInput.hpp"
#include "CustomSlider.hpp"
#include "Duration.hpp"
#include "Enums.hpp"
#include "EqualizerMenu.hpp"
#include "IconTextButton.hpp"
#include "InputPopup.hpp"
#include "MetadataWindow.hpp"
#include "OptionMenu.hpp"
#include "PlaylistTabBar.hpp"
#include "Settings.hpp"
#include "SettingsWindow.hpp"
#include "SpectrumVisualizer.hpp"
#include "TrackRepeatMenu.hpp"
#include "TrackTable.hpp"
#include "TrackTableModel.hpp"
#include "Utils.hpp"
#include "ui_MainWindow.h"
#include "version.h"

#ifdef PROJECTM
#include "VisualizerDialog.hpp"
#endif

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>
#include <QString>
#include <QStyleFactory>
#include <QThreadPool>
#include <QTranslator>
#include <QVersionNumber>
#include <QWidgetAction>
#include <QXmlStreamReader>

#ifdef Q_OS_WINDOWS
#include <bit7z/bitarchivereader.hpp>
using namespace bit7z;
#elifdef Q_OS_LINUX
// we're using tar
#elifdef Q_OS_MACOS
#endif

// TODO: Support for global shortcuts on Windows/Linux
// TODO: Add dB volume slider mode
// TODO: Implement setting to not load metadata before playing
// TODO: Implement desktop notifications
// TODO: Add `dynamic` option for tray icon, and change its look depending on
// the state if on.
// TODO: Allow user to see and modify playback history.
// TODO: Add option to select between displaying leading zero in durations.
// TODO: Add option to explicitly show hours in durations.
// TODO: Add option for setting custom displayed track title format.
// TODO: Implement soft clipping.
// TODO: Don't remove unavailable entries from playlists. Notify the user and
// let them do this. Add option to purge unavailable entries. Mark unavailable
// entries as red, or even better let the user decide.
// TODO: Implement track queue.
// TODO: Implement "stop after this track".
// TODO: Possibly add info bar for the played track in the main window.
// TODO: Add feature to mark tracks in playlist as favorites.
// TODO: Add explicit "play in rap" and "queue in rap" context menu entries.
// When the latter is selected, start playing the folder. When the former is
// selected, just add to the playlists.
// TODO: When using "queue in rap", how to handle queuing? It should be possible
// to add playlist to the end of all playlists, and queue it next.
// TODO: Change playlist view indexing to use i32 instead of i8/u8?

MainWindow::MainWindow(const QStringList& paths, QWidget* const parent) :
    QMainWindow(parent),

    ui(setupUi()),

    statusBar(ui->statusBar),
    playlistTracksStatus(new QLabel(ui->statusBar)),
    playlistDurationStatus(new QLabel(ui->statusBar)),
    currentTrackStatus(new QLabel(ui->statusBar)),

    trayIcon(new QSystemTrayIcon(this)),
    trayIconMenu(new OptionMenu(this)),
    progressLabelTray(new ProgressLabel(trayIconMenu)),
    volumeLabelTray(new QLabel(trayIconMenu)),
    volumeSliderTray(new CustomSlider(Qt::Horizontal, trayIconMenu)),
    progressSliderTray(new CustomSlider(Qt::Horizontal, trayIconMenu)),

    progressLabel(ui->progressLabel),
    volumeLabel(ui->volumeLabel),
    trackLabel(ui->trackLabel),

    playlistView(ui->playlistView),
    playlistTabBar(playlistView->tabBar()),

    playButton(ui->playButton),
    stopButton(ui->stopButton),
    backwardButton(ui->backwardButton),
    forwardButton(ui->forwardButton),
    repeatButton(ui->repeatButton),
    randomButton(ui->randomButton),
    equalizerButton(ui->equalizerButton),

    volumeSlider(ui->volumeSlider),
    progressSlider(ui->progressSlider),

    pauseIcon(QIcon::fromTheme(u"media-playback-pause"_s)),
    startIcon(QIcon::fromTheme(u"media-playback-start"_s)),

    actionOpenFile(ui->actionOpenFile),
    actionOpenFolder(ui->actionOpenFolder),
    actionOpenPlaylist(ui->actionOpenPlaylist),
    actionSettings(ui->actionSettings),
    actionVisualizer(ui->actionVisualizer),
    actionExit(ui->actionExit),

    actionAddFile(ui->actionAddFile),
    actionAddFolder(ui->actionAddFolder),
    actionAddPlaylist(ui->actionAddPlaylist),

    actionExportXSPFPlaylist(ui->actionExportXSPFPlaylist),
    actionExportM3U8Playlist(ui->actionExportM3U8Playlist),

    actionRussian(ui->actionRussian),
    actionEnglish(ui->actionEnglish),

    actionAbout(ui->actionAbout),
    actionDocumentation(ui->actionDocumentation),
    actionCheckForUpdates(ui->actionCheckForUpdates),

    actionForward(ui->actionForward),
    actionBackward(ui->actionBackward),
    actionRepeat(ui->actionRepeat),
    actionTogglePlayback(ui->actionTogglePlayback),
    actionStop(ui->actionStop),
    actionRandom(ui->actionRandom),

    IPCServerName(u"revolutionary-audio-player-server"_s),

    translator(new QTranslator(this)),

    mainArea(ui->mainArea),
    dockWidget(ui->dockWidget),
    dockCoverLabel(ui->dockCoverLabel),
    dockMetadataTree(ui->dockMetadataTree),

    threadPool(new QThreadPool()),

    spectrumVisualizer(nullptr),
    audioWorker(nullptr),

    searchTrackInput(new CustomInput(this)),
    searchMatchesPosition(0),
    searchShortcut(new QShortcut(QKeySequence(u"Ctrl+F"_s), this)),

    server(new QLocalServer(this)),

    trackRepeatMenu(new TrackRepeatMenu(this)) {
    statusBar->addPermanentWidget(playlistTracksStatus);
    statusBar->addPermanentWidget(playlistDurationStatus);
    statusBar->addPermanentWidget(currentTrackStatus);

    dockWidget->dockCoverLabel = dockCoverLabel;
    dockWidget->dockMetadataTree = dockMetadataTree;

    for (const auto prop : TRACK_PROPERTIES) {
        auto* const item = new QTreeWidgetItem();
        item->setText(0, trackPropertyLabel(prop));
        dockMetadataTree->addTopLevelItem(item);
    }

    dockMetadataTree->setFocusPolicy(Qt::StrongFocus);
    dockWidget->setFocusPolicy(Qt::NoFocus);
    dockWidget->setFocusProxy(dockMetadataTree);

    connect(
        equalizerButton,
        &QPushButton::pressed,
        this,
        &MainWindow::toggleEqualizerMenu
    );

    connect(actionForward, &QAction::triggered, this, [this] -> void {
        advancePlaylist(PlaylistView::Direction::Forward);
    });

    connect(actionBackward, &QAction::triggered, this, [this] -> void {
        advancePlaylist(PlaylistView::Direction::Backward);
    });

    connect(actionRepeat, &QAction::triggered, this, &MainWindow::toggleRepeat);

    connect(actionTogglePlayback, &QAction::triggered, this, [this] -> void {
        togglePlayback();
    });

    connect(actionStop, &QAction::triggered, this, &MainWindow::stopPlayback);

    connect(
        actionRandom,
        &QAction::triggered,
        playlistView,
        &PlaylistView::toggleRandom
    );

    connect(
        actionVisualizer,
        &QAction::triggered,
        this,
        &MainWindow::showVisualizer
    );

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

    connect(actionOpenFolder, &QAction::triggered, this, [this] -> void {
        addEntry(true, true);
    });

    connect(actionOpenFile, &QAction::triggered, this, [this] -> void {
        addEntry(true, false);
    });

    connect(actionAddFolder, &QAction::triggered, this, [this] -> void {
        addEntry(false, true);
    });

    connect(actionAddFile, &QAction::triggered, this, [this] -> void {
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
        [this](const u32 seconds) -> void {
        progressLabel->setElapsed(seconds);
        progressLabelTray->setElapsed(seconds);
    }
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
        [this](const u32 seconds) -> void {
        progressLabel->setElapsed(seconds);
        progressLabelTray->setElapsed(seconds);
    }
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

    connect(searchShortcut, &QShortcut::activated, this, [this] -> void {
        toggleSearchInput();
    });

    connect(actionDocumentation, &QAction::triggered, this, [] -> void {
        QDesktopServices::openUrl(
            QUrl(u"https://savannstm.github.io/revolutionary-audio-player"_s)
        );
    });

    connect(actionCheckForUpdates, &QAction::triggered, this, [this] -> void {
        checkForUpdates(true);
    });

    connect(actionOpenPlaylist, &QAction::triggered, this, [this] -> void {
        importPlaylist(true);
    });

    connect(actionAddPlaylist, &QAction::triggered, this, [this] -> void {
        importPlaylist(false);
    });

    connect(
        actionExportM3U8Playlist,
        &QAction::triggered,
        this,
        [this] -> void { exportPlaylist(PlaylistFileType::M3U); }
    );

    connect(
        actionExportXSPFPlaylist,
        &QAction::triggered,
        this,
        [this] -> void { exportPlaylist(PlaylistFileType::XSPF); }
    );

    connect(
        playlistView,
        &PlaylistView::indexChanged,
        this,
        &MainWindow::changePlaylist
    );

    connect(actionRussian, &QAction::triggered, this, [this] -> void {
        retranslate(QLocale::Russian);
    });

    connect(actionEnglish, &QAction::triggered, this, [this] -> void {
        retranslate(QLocale::English);
    });

    connect(
        dockWidget,
        &DockWidget::repositioned,
        this,
        &MainWindow::moveDockWidget
    );

    connect(
        server,
        &QLocalServer::newConnection,
        this,
        &MainWindow::handleConnectionIPC
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

    connect(
        ui->controlContainer,
        &QWidget::customContextMenuRequested,
        this,
        &MainWindow::showControlContainerContextMenu
    );

    connect(
        repeatButton,
        &IconTextButton::customContextMenuRequested,
        this,
        &MainWindow::showRepeatButtonContextMenu
    );

    connect(
        playlistView,
        &PlaylistView::playingChanged,
        this,
        &MainWindow::playTrack,
        Qt::UniqueConnection
    );

    connect(
        playlistView,
        &PlaylistView::trackPressed,
        this,
        &MainWindow::handleTrackPress
    );

    connect(
        playlistView,
        &PlaylistView::rowsRemoved,
        this,
        &MainWindow::updateStatusBar
    );

    connect(
        playlistView,
        &PlaylistView::rowsInserted,
        this,
        &MainWindow::updateStatusBar
    );

    connect(
        playlistView,
        &PlaylistView::layoutChanged,
        this,
        &MainWindow::updateStatusBar
    );

#ifdef Q_OS_LINUX
    // On Linux, server cannot listen to the given name if it's already exists.
    // Since everything is file in Unix systems, server is persistent across
    // runs.
    QLocalServer::removeServer(IPCServerName);
#endif

    server->listen(IPCServerName);

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
    audioWorker = new AudioWorker(settings);
    equalizerMenu = new EqualizerMenu(settings, this);

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

    spectrumVisualizer = new SpectrumVisualizer(settings, this);

    connect(
        audioWorker,
        &AudioWorker::fftSamples,
        spectrumVisualizer,
        [this](const span<const f32> samples, const u32 sampleRate) -> void {
        array<f32, FFT_SAMPLE_COUNT> downsampled;

        if (sampleRate <= FFT_SAMPLE_RATE) {
            memcpy(
                downsampled.data(),
                samples.data(),
                samples.size() * F32_SIZE
            );
        } else {
            downsample(
                samples.data(),
                downsampled.data(),
                samples.size(),
                sampleRate
            );
        }

        spectrumVisualizer->visualize(downsampled.data());

        if (visualizerDialog != nullptr) {
            visualizerDialog->addSamples(downsampled.data());
        }
    }
    );

    ui->controlLayout->insertWidget(
        ui->controlLayout->indexOf(equalizerButton) + 1,
        spectrumVisualizer
    );

    connect(
        spectrumVisualizer,
        &SpectrumVisualizer::closed,
        this,
        &MainWindow::reacquireSpectrumVisualizer
    );

    connect(
        progressLabel,
        &ProgressLabel::modeChanged,
        this,
        [this](const ProgressDisplayMode mode) -> void {
        settings->core.progressDisplayMode = mode;
    }
    );

    QFile::remove(qApp->applicationDirPath() + u"/rap-old.exe"_qssv);

    if (settings->core.checkForUpdates) {
        checkForUpdates();
    }

    processArgs(paths);
}

MainWindow::~MainWindow() {
    delete ui;
    delete audioWorker;
    delete threadPool;
}

void MainWindow::focus() {
    show();
    raise();

    Qt::WindowStates state = windowState();
    state.setFlag(Qt::WindowMinimized, false);
    setWindowState(state);
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

        TrackTable* const table = playlistView->table(index);

        connect(
            table,
            &TrackTable::fillingFinished,
            this,
            [this, table](const bool startPlaying) -> void {
            if (startPlaying) {
                playlistView->setPlayingIndex(
                    table,
                    table->model()->index(0, 0)
                );
            }
        },
            Qt::SingleShotConnection
        );

        table->fill(args, {}, true);
        playlistView->setCurrentIndex(i8(index));
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

void MainWindow::reacquireSpectrumVisualizer() {
    spectrumVisualizer->setParent(ui->controlContainer);
    ui->controlLayout->insertWidget(
        ui->controlLayout->indexOf(equalizerButton) + 1,
        spectrumVisualizer
    );
    spectrumVisualizer->show();
}

void MainWindow::dropEvent(QDropEvent* const event) {
    const QString data = event->mimeData()->text();

    if (data == "spectrumvisualizer"_L1) {
        reacquireSpectrumVisualizer();
        return;
    }

    const QPoint dropPos = event->position().toPoint();
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

        if (ranges::contains(SUPPORTED_PLAYLIST_EXTENSIONS, extension)) {
            playlistPaths.append(path);
            continue;
        }

        if (ranges::contains(SUPPORTED_PLAYABLE_EXTENSIONS, extension)) {
            filePaths.append(path);
        }
    }

    TrackTable* table = playlistView->currentTable();

    const bool outsideTable =
        table == nullptr ||
        !QRect(table->mapToGlobal(QPoint(0, 0)), table->size())
             .contains(dropPos);

    if (outsideTable) {
        for (const QString& dirPath : dirPaths) {
            const auto info = QFileInfo(dirPath);
            auto* newTable =
                playlistView->table(playlistView->addTab(info.fileName()));
            newTable->fill({ dirPath });
        }

        for (const QString& playlistPath : playlistPaths) {
            importPlaylist(true, playlistPath);
        }

        if (!filePaths.isEmpty()) {
            const auto info = QFileInfo(filePaths[0]);

            table =
                playlistView->table(playlistView->addTab(info.dir().dirName()));

            table->fill(filePaths);
        }
    } else {
        table->fill(dirPaths);

        if (!filePaths.isEmpty()) {
            table->fill(filePaths);
        }

        for (const QString& playlistPath : playlistPaths) {
            importPlaylist(false, playlistPath);
        }
    }
}

void MainWindow::updatePlaybackPosition() {
    i32 second = progressSlider->value();

    if (CUEOffset != -1) {
        second += CUEOffset;
    }

    audioWorker->seekSecond(second);
    progressLabel->setElapsed(second);
    progressLabelTray->setElapsed(second);
}

void MainWindow::loadPlaylists() {
    auto playlistsFile = QFile(
        settings->core.playlistsDir + PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
    );

    if (!playlistsFile.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << playlistsFile.errorString();
        return;
    }

    const QJsonArray playlistsArray =
        QJsonDocument::fromJson(playlistsFile.readAll()).array();

    for (const auto playlistObject : playlistsArray) {
        const PlaylistObject playlist =
            PlaylistObject::fromJSON(playlistObject.toObject());

        const u8 index =
            playlistView->addTab(playlist.tabLabel, playlist.columns);

        playlistView->setTabColor(index, playlist.tabColor);

        TrackTable* const table = playlistView->table(index);
        table->setOpacity(playlist.backgroundOpacity);

        if (!playlist.order.empty()) {
            connect(
                table,
                &TrackTable::fillingFinished,
                this,
                [=](const bool /* startPlaying */) -> void {
                for (const i32 track : range(0, table->model()->rowCount())) {
                    table->model()->setOrder(track, playlist.order[track]);
                }
            },
                Qt::SingleShotConnection
            );
        }

        table->fill(playlist.tracks, playlist.cueOffsets);

        const QString& imagePath = playlist.backgroundImagePath;

        if (!imagePath.isEmpty()) {
            const QString extension =
                imagePath.sliced(imagePath.lastIndexOf('.') + 1);

            QImage image;

            if (ranges::contains(SUPPORTED_AUDIO_EXTENSIONS, extension)) {
                const auto extracted = extractCover(imagePath.toUtf8().data());

                if (!extracted) {
                    qWarning() << extracted.error();
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
    auto playlistsFile = QFile(
        settings->core.playlistsDir + PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
    );

    if (!playlistsFile.open(QFile::WriteOnly)) {
        return Err(playlistsFile.errorString());
    }

    QJsonArray playlistsArray;

    for (const u8 tab : range<u8>(0, playlistView->tabCount())) {
        TrackTable* const table = playlistView->table(tab);
        const TrackTableModel* const model = table->model();

        const i32 rowCount = model->rowCount();

        PlaylistObject playlistObject = PlaylistObject::withRowCount(rowCount);
        playlistObject.tabLabel = playlistView->tabLabel(tab);
        playlistObject.backgroundImagePath =
            playlistView->backgroundImagePath(tab);
        playlistObject.tabColor = playlistView->tabColor(tab);
        playlistObject.backgroundOpacity = table->opacity();

        for (const i32 row : range<i32>(0, rowCount)) {
            const TrackMetadata rowMetadata = table->rowMetadata(row);
            const QString& cueFilePath = model->cueFilePath(row);
            const u32 cueOffset = model->cueOffset(row);

            playlistObject.tracks.append(
                cueFilePath.isEmpty() ? rowMetadata[TrackProperty::Path]
                                      : cueFilePath
            );
            playlistObject.order.append(rowMetadata[TrackProperty::Order]);
            playlistObject.cueOffsets.append(cueOffset);
        }

        playlistObject.columns = table->columnSettings();
        playlistsArray.append(playlistObject.toJSON());
    }

    playlistsFile.write(
        QJsonDocument(playlistsArray).toJson(QJsonDocument::Compact)
    );
    return true;
}

auto MainWindow::saveSettings() -> result<bool, QString> {
    auto settingsFile = QFile(
        settings->core.settingsDir + SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                         .toString()
#endif
    );

    if (!settingsFile.open(QFile::WriteOnly)) {
        return Err(settingsFile.errorString());
    }

    settings->core.currentTab = playlistView->currentIndex();
    settings->core.volume = volumeSlider->value();

    equalizerMenu->saveSettings();

    DockWidget::Position dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidget::Position::Left
                                                  : DockWidget::Position::Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0
                                 ? DockWidget::Position::Top
                                 : DockWidget::Position::Bottom;
    }

    settings->dockWidget.position = dockWidgetPosition;
    settings->dockWidget.size = dockWidget->width();
    settings->dockWidget.imageSize = dockCoverLabel->height();

    settings->save(settingsFile);

    auto rapPathsFile = QFile(
        qApp->applicationDirPath() + PATHS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                         .toString()
#endif
    );

    if (rapPathsFile.open(QFile::WriteOnly | QFile::Truncate)) {
        rapPathsFile.write(settings->core.settingsDir.toUtf8());
        rapPathsFile.write("\n");
        rapPathsFile.write(settings->core.playlistsDir.toUtf8());
    }

    return savePlaylists();
}

void MainWindow::retranslate(const QLocale::Language language) {
    qApp->removeTranslator(translator);
    delete translator;

    translator = new QTranslator(this);
    const bool success = translator->load(
        ":/%1.qm"_L1.arg(QLocale(language).bcp47Name().split('-').first())
    );

    qApp->installTranslator(translator);

    ui->retranslateUi(this);

    const u8 tabCount = playlistView->tabCount();

    for (const u8 tab : range<u8>(0, tabCount)) {
        TrackTable* const table = playlistView->table(tab);
        TrackTableModel* const model = table->model();

        for (const auto prop : TRACK_PROPERTIES) {
            model->setHeaderData(
                u8(prop),
                Qt::Horizontal,
                trackPropertyLabel(prop)
            );
            table->resizeColumnToContents(u8(prop));
        }

        table->setColumnWidth(0, MINIMUM_TRACK_TABLE_COLUMN_SECTION_SIZE);
    }

    equalizerMenu->retranslate();
    searchTrackInput->setPlaceholderText(tr("Track name/property"));

    if (audioWorker->state() == ma_device_state_started) {
        playButton->setToolTip(tr("Pause"));
        actionTogglePlayback->setText(tr("Pause"));
    } else {
        playButton->setToolTip(tr("Play"));
        actionTogglePlayback->setText(tr("Play"));
    }
}

void MainWindow::initializeSettings() {
    auto rapPathsFile = QFile(
        qApp->applicationDirPath() + PATHS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                         .toString()
#endif
    );
    QFile* settingsFile;
    QString settingsFilePath;
    QString playlistsFilePath;

    if (!rapPathsFile.open(QFile::ReadOnly)) {
        qWarning() << rapPathsFile.errorString();

        settingsFilePath = qApp->applicationDirPath() + SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                                            .toString()
#endif
            ;
        playlistsFilePath = qApp->applicationDirPath() + PLAYLISTS_FILE_NAME
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

        settingsFilePath = settingsDir + SETTINGS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                             .toString()
#endif
            ;
        playlistsFilePath = playlistsDir + PLAYLISTS_FILE_NAME
#if QT_VERSION_MINOR < 9
                                               .toString()
#endif
            ;

        settingsFile = new QFile(settingsFilePath);
    }

    if (!settingsFile->open(QFile::ReadOnly)) {
        qWarning() << settingsFile->fileName() + settingsFile->errorString();

        settings = make_shared<Settings>();
        delete settingsFile;

        return;
    }

    const QByteArray jsonData = settingsFile->readAll();

    QJsonParseError jsonError;
    const QJsonObject settingsObject =
        QJsonDocument::fromJson(jsonData, &jsonError).object();

    if (jsonError.error != QJsonParseError::NoError) {
        qWarning() << jsonError.errorString();
    } else {
        settings = make_shared<Settings>(Settings::fromJSON(settingsObject));
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

    settings->core.defaultStyle = qApp->style()->name();
    qApp->setStyle(settings->core.applicationStyle);

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
        updateFileAssociationsPath();
    }

    moveDockWidget(settings->dockWidget.position);
    mainArea->setSizes({ QWIDGETSIZE_MAX, settings->dockWidget.size });

    retranslate(settings->core.language);
}

void MainWindow::moveDockWidget(const DockWidget::Position dockWidgetPosition) {
    Qt::Orientation targetMainOrientation;
    Qt::Orientation targetDockOrientation;
    u8 targetIndex;

    switch (dockWidgetPosition) {
        case DockWidget::Position::Left:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 0;
            break;
        case DockWidget::Position::Right:
            targetMainOrientation = Qt::Horizontal;
            targetDockOrientation = Qt::Vertical;
            targetIndex = 1;
            break;
        case DockWidget::Position::Top:
            targetMainOrientation = Qt::Vertical;
            targetDockOrientation = Qt::Horizontal;
            targetIndex = 0;
            break;
        case DockWidget::Position::Bottom:
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

    TrackTable* const table = playlistView->currentTable();
    TrackTableModel* const tableModel = table->model();

    switch (qApp->mouseButtons()) {
        case Qt::RightButton: {
            const QModelIndex currentIndex = table->currentIndex();

            auto* const menu = new QMenu(this);

            const QAction* const removeAction =
                menu->addAction(tr("Remove Track"));

            const QModelIndexList selectedRows =
                table->selectionModel()->selectedRows();
            const QAction* removeSelectionAction = nullptr;

            if (selectedRows.size() > 2) {
                removeSelectionAction = menu->addAction(tr("Remove Selection"));
            }

            const QAction* const clearAction =
                menu->addAction(tr("Clear All Tracks"));

            menu->addSeparator();

            const QAction* const metadataAction =
                menu->addAction(tr("Show Track Metadata"));
            const QAction* const coverAction =
                menu->addAction(tr("Show Cover"));

            menu->addSeparator();

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
                tableModel->removeRow(index.row());

                table->setCurrentIndex(
                    index.row() > currentIndex.row()
                        ? currentIndex
                        : tableModel->index(currentIndex.row() - 1, 0)
                );
            } else if (selectedAction == clearAction) {
                tableModel->removeRows(0, tableModel->rowCount());
            } else if (selectedAction == metadataAction) {
                auto* const metadataWindow =
                    new MetadataWindow(table->rowMetadata(index.row()), this);
                metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                metadataWindow->show();
            } else if (selectedAction == coverAction) {
                const TrackMetadata metadata = table->rowMetadata(index.row());

                auto* const coverWindow = new CoverWindow(
                    metadata[TrackProperty::Path],
                    metadata[TrackProperty::Title]
                );
                coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                coverWindow->show();
            } else if (selectedAction == removeSelectionAction) {
                const QModelIndex& startRow = selectedRows.first();
                const isize count = selectedRows.size();

                tableModel->removeRows(startRow.row(), i32(count));

                table->setCurrentIndex(
                    index.row() > currentIndex.row()
                        ? currentIndex
                        : tableModel->index(
                              i32(currentIndex.row() - selectedRows.size()),
                              0
                          )
                );
            } else if (selectedAction == setPlaylistBackground) {
                QString filter = tr("Image Files");
                filter.reserve(
                    3 + (SUPPORTED_IMAGE_EXTENSIONS.size() * 4) +
                    (SUPPORTED_IMAGE_EXTENSIONS.size() * 3)
                );

                filter += " ("_L1;

                for (const auto ext : SUPPORTED_IMAGE_EXTENSIONS) {
                    filter += "*."_L1;
                    filter += ext;
                    filter += ' ';
                }

                filter.removeLast();
                filter += ')';

                const QString file = QFileDialog::getOpenFileName(
                    this,
                    tr("Select File"),
                    QString(),
                    filter
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
                    QString::number(table->opacity(), 'f', 2),
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
                    table,
                    [table](const QString& text) -> void {
                    table->setOpacity(QLocale::system().toFloat(text));
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

    TrackTable* const table = playlistView->currentTable();
    TrackTableModel* const tableModel = table->model();

    if (searchMatches.empty()) {
        const isize colonPos = input.indexOf(':');

        QString prefix;
        QString value;

        bool searchByTag = false;
        TrackProperty property = TrackProperty::Title;

        if (colonPos != -1) {
            prefix = input.sliced(0, colonPos);
            value = input.sliced(colonPos + 1);
            searchByTag = true;

            bool tagFound = false;

            for (const auto [idx, info] :
                 views::enumerate(TRACK_PROPERTIES_INFOS)) {
                if (prefix == info.tag) {
                    tagFound = true;
                    property = TrackProperty(idx);
                    break;
                }
            }

            if (!tagFound) {
                searchTrackInput->clear();
                searchTrackInput->setPlaceholderText(tr("Incorrect tag!"));
            }
        }

        for (const i32 row : range(0, tableModel->rowCount())) {
            const TrackMetadata metadata = table->rowMetadata(row);
            QString fieldValue;

            if (searchByTag) {
                fieldValue = metadata[property];
            } else {
                fieldValue = metadata[TrackProperty::Title];
                value = input;
            }

            const QString fieldValueLower = fieldValue.toLower();
            const QString valueLower = value.toLower();

            if (fieldValue.contains(value) ||
                fieldValueLower.contains(valueLower)) {
                searchMatches.append(tableModel->index(row, 0));
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

    table->selectionModel()->clearSelection();

    const QModelIndex& match = searchMatches[searchMatchesPosition++];
    table->scrollTo(match, TrackTable::PositionAtCenter);
    table->selectionModel()->select(
        match,
        QItemSelectionModel::Select | QItemSelectionModel::Rows
    );
}

void MainWindow::toggleSearchInput(const bool forceShow) {
    TrackTable* const table = playlistView->currentTable();

    if (table == nullptr) {
        return;
    }

    if (searchTrackInput->isHidden() || forceShow) {
        const u16 xPos = mainArea->width() - dockWidget->width() -
                         searchTrackInput->width() -
                         SEARCH_INPUT_POSITION_PADDING;
        const u16 yPos = 0 + SEARCH_INPUT_POSITION_PADDING;

        searchTrackInput->move(table->mapToGlobal(QPoint(xPos, yPos)));
        searchTrackInput->raise();
        searchTrackInput->show();
        searchTrackInput->setFocus();
    } else {
        searchTrackInput->hide();
    }
}

void MainWindow::onAudioProgressUpdated(i32 seconds) {
    const bool draggingSlider =
        progressSlider->isSliderDown() || progressSliderTray->isSliderDown();

    if (draggingSlider) {
        return;
    }

    if (playlistView->repeatMode() == PlaylistView::RepeatMode::Track) {
        const vector<SkipSection>& skipSections =
            trackRepeatMenu->skipSections();

        for (const auto section : skipSections) {
            u32 start = section.start;
            u32 end = section.end;

            if (CUEOffset != -1) {
                start += CUEOffset;
                end += CUEOffset;
            }

            if (seconds > start && seconds <= end) {
                audioWorker->seekSecond(end);
            }
        }

        u32 startSecond = trackRepeatMenu->startSecond();
        u32 endSecond = trackRepeatMenu->endSecond();

        if (CUEOffset != -1) {
            startSecond += CUEOffset;
            endSecond += CUEOffset;
        }

        if (seconds < startSecond) {
            audioWorker->seekSecond(startSecond);
        }

        if (seconds > endSecond - 1) {
            audioWorker->seekSecond(startSecond);
        }
    }

    const u16 secsDuration = progressLabel->durationSeconds();

    if (CUEOffset != -1) {
        seconds -= CUEOffset;

        if (seconds == secsDuration) {
            playNext();
        }
    }

    progressSlider->setValue(seconds);
}

void MainWindow::playNext() {
    switch (playlistView->repeatMode()) {
        case PlaylistView::RepeatMode::Track:
            audioWorker->seekSecond(0);
            break;
        default:
            advancePlaylist(PlaylistView::Direction::Forward);
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

    TrackTable* table = playlistView->currentTable();

    if (createNewTab || table == nullptr) {
        const u8 index = playlistView->addTab(
            settings->playlist.playlistNaming == PlaylistNaming::DirectoryName
                ? QFileInfo(path).fileName()
                : QString(),
            settings->playlist.columns
        );
        table = playlistView->table(index);
    }

    table->fill({ path });
}

void MainWindow::showAboutWindow() {
    auto* const aboutWindow = new AboutWindow(this);
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
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

void MainWindow::exit() {
    const auto saveResult = saveSettings();

    if (saveResult) {
        qApp->quit();
    } else {
        QMessageBox messageBox = QMessageBox(this);
        messageBox.setWindowTitle(tr("Save Failed!"));
        messageBox.setText(saveResult.error());
        messageBox.setIcon(QMessageBox::Warning);

        const QPushButton* const tryAgainButton =
            messageBox.addButton(tr("Try Again"), QMessageBox::AcceptRole);
        const QPushButton* const cancelButton =
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
        activateWindow();
        raise();
        focus();
    }
}

void MainWindow::setupTrayIcon() {
    trayIcon->setIcon(QIcon(u":/logo.png"_s));
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

void MainWindow::updateStatusBar() {
    TrackTable* const table = playlistView->currentTable();
    TrackTableModel* const tableModel = table->model();

    if (tableModel != nullptr) {
        playlistTracksStatus->setText(
            tr("%1 Tracks").arg(tableModel->rowCount())
        );
        playlistDurationStatus->setText(
            table->duration().toString("hh:mm:ss"_L1)
        );

        const QModelIndex index = table->currentIndex();

        if (index.isValid()) {
            currentTrackStatus->setText(
                tr("Playing Row: %1").arg(index.row() + 1)
            );
        } else {
            currentTrackStatus->setText(tr("Not Playing"));
        }
    }
}

auto MainWindow::changePlaylist(const i8 index) -> bool {
    if (index == -1) {
        playlistTracksStatus->setText("");
        playlistDurationStatus->setText("");
        return false;
    }

    TrackTable* const table = playlistView->table(index);
    TrackTableModel* const model = table->model();

    playlistTracksStatus->setText(tr("%1 Tracks").arg(model->rowCount()));
    playlistDurationStatus->setText(table->duration().toString("hh:mm:ss"_L1));

    return true;
}

void MainWindow::playTrack(TrackTable* const table, const i32 row) {
    if (row == -1) {
        return;
    }

    const TrackMetadata metadata = table->rowMetadata(row);

    const bool playingCUE =
        metadata[TrackProperty::AudioFormat].toLower().endsWith(EXT_CUE);

    if (playingCUE) {
        CUEOffset = table->model()->cueOffset(row);
    }

    togglePlayback(metadata[TrackProperty::Path], playingCUE ? CUEOffset : -1);

    QString artistAndTrack;
    artistAndTrack.reserve(isize(
        metadata[TrackProperty::Artist].size() + (sizeof(" - ") - 1) +
        metadata[TrackProperty::Title].size()
    ));
    artistAndTrack += metadata[TrackProperty::Artist];
    artistAndTrack += " - "_L1;
    artistAndTrack += metadata[TrackProperty::Title];
    trackLabel->setText(artistAndTrack);

    QString windowTitle;
    windowTitle.reserve(isize(sizeof("RAP: ") - 1) + artistAndTrack.size());
    windowTitle += "RAP: "_L1;
    windowTitle += artistAndTrack;
    setWindowTitle(windowTitle);

    for (const auto prop : TRACK_PROPERTIES) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(u8(prop), 0))
            ->setText(
                1,
                prop == TrackProperty::Duration
                    ? Duration::secondsToString(metadata[prop].toUInt())
                    : metadata[prop]
            );
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

            if (!ranges::contains(SUPPORTED_IMAGE_EXTENSIONS, extension)) {
                continue;
            }

            const QString prefix = entry.sliced(0, 5).toLower();

            if (prefix != "cover"_L1) {
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
            qWarning() << extracted.error();
        } else {
            const vector<u8>& coverBytes = extracted.value();
            cover.loadFromData(coverBytes.data(), coverBytes.size());
            path = metadata[TrackProperty::Path];
        }
    }

    if (settings->playlist.autoSetBackground) {
        const QImage image = cover.toImage();
        playlistView
            ->setBackgroundImage(playlistView->currentIndex(), image, path);
    }

    const i32 seconds =
        Duration::stringToSeconds(metadata[TrackProperty::Duration]).value();

    trackRepeatMenu->setDuration(seconds);

    progressSlider->setRange(0, seconds);
    progressSliderTray->setRange(0, seconds);

    progressSlider->setValue(0);
    progressSliderTray->setValue(0);

    progressLabel->setDuration(seconds);
    progressLabelTray->setDuration(seconds);

    progressLabel->setElapsed(0);
    progressLabelTray->setElapsed(0);

    dockCoverLabel->clear();
    dockCoverLabel->setPixmap(cover);

    currentTrackStatus->setText(tr("Playing Row: %1").arg(row + 1));
    statusBar->showMessage(
        tr("Started playback of %1.").arg(metadata[TrackProperty::Title]),
        STATUSBAR_MSG_TIMEOUT
    );
}

void MainWindow::togglePlayback(const QString& path, const i32 startSecond) {
    if (!path.isEmpty()) {
        if (trackLabel->text() == path) {
            audioWorker->seekSecond(startSecond);
            return;
        }

        audioWorker->startPlayback(path, startSecond);
        playButton->setIcon(pauseIcon);
        playButton->setToolTip(tr("Pause"));
        actionTogglePlayback->setText(tr("Pause"));
    } else {
        const ma_device_state state = audioWorker->state();

        if (state == ma_device_state_started) {
            audioWorker->pause();
            playButton->setIcon(startIcon);
            playButton->setToolTip(tr("Play"));
            actionTogglePlayback->setText(tr("Play"));
        } else if (state == ma_device_state_stopped &&
                   playlistView->playingTable() != nullptr) {
            audioWorker->resume();

            playButton->setIcon(pauseIcon);
            playButton->setToolTip(tr("Pause"));
            actionTogglePlayback->setText(tr("Pause"));
        } else if (state == ma_device_state_uninitialized ||
                   state == ma_device_state_stopped) {
            TrackTable* const table = playlistView->currentTable();

            if (table == nullptr) {
                return;
            }

            playlistView->setPlayingIndex(table, table->model()->index(0, 0));
        }
    }
}

void MainWindow::advancePlaylist(const PlaylistView::Direction direction) {
    const AdvanceResult result = playlistView->advance(direction);

    switch (result.status) {
        case AdvanceStatus::PlaybackFinished:
            statusBar->showMessage(
                tr("Playlists are consumed, playback is finished.")
            );
            break;
        case AdvanceStatus::PlaylistFinished:
            statusBar->showMessage(
                tr("Playlist is consumed, moving to the next one."),
                STATUSBAR_MSG_TIMEOUT
            );
            break;
        case AdvanceStatus::TrackFinished:
            statusBar->showMessage(
                tr("Track is finished."),
                STATUSBAR_MSG_TIMEOUT
            );
            break;
        case AdvanceStatus::TrackRepeated:
            statusBar->showMessage(
                tr("Repeating track."),
                STATUSBAR_MSG_TIMEOUT
            );
            break;
        case AdvanceStatus::PlaylistRepeated:
            statusBar->showMessage(
                tr("Repeating playlist."),
                STATUSBAR_MSG_TIMEOUT
            );
            break;
    }

    if (result.table == nullptr) {
        stopPlayback();
        return;
    }

    playTrack(result.table, result.index.row());
}

void MainWindow::stopPlayback() {
    spectrumVisualizer->reset();
    audioWorker->stopPlayback();

    trackLabel->clear();

    progressSlider->setRange(0, 0);
    progressSliderTray->setRange(0, 0);

    progressLabel->setDuration(0);
    progressLabelTray->setDuration(0);

    progressLabel->setElapsed(0);
    progressLabelTray->setElapsed(0);

    playButton->setIcon(startIcon);
    playButton->setToolTip(tr("Play"));
    actionTogglePlayback->setText(tr("Play"));

    playlistView->resetPlayingIndex();

    setWindowTitle(u"RAP"_s);

    dockCoverLabel->clear();

    for (const auto prop : TRACK_PROPERTIES) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(u8(prop), 0))
            ->setText(1, QString());
    }
}

void MainWindow::importPlaylist(const bool createNewTab, QString filePath) {
    if (filePath.isEmpty()) {
        QString filter = tr("XSPF/M3U/CUE Playlist");
        filter.reserve(2 + (SUPPORTED_PLAYLIST_EXTENSIONS_COUNT * 7) + 1);
        filter += " ("_L1;

        for (const QLatin1StringView ext : SUPPORTED_PLAYLIST_EXTENSIONS) {
            filter += "*."_L1;
            filter += ext;
            filter += ' ';
        }

        filter.removeLast();
        filter += ')';

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
        CUEInfo info = parseCUE(file, fileInfo);
        auto& metadata = info.metadata;

        QString tabName;
        tabName.reserve(isize(
            metadata[TrackProperty::Artist].size() + (sizeof(" - ") - 1) +
            metadata[TrackProperty::Title].size()
        ));
        tabName += metadata[TrackProperty::Artist];
        tabName += " - "_L1;
        tabName += metadata[TrackProperty::Title];

        const i8 currentIndex = playlistView->currentIndex();
        const u8 index = (createNewTab || currentIndex == -1)
                             ? playlistView->addTab(tabName)
                             : currentIndex;

        playlistView->table(index)
            ->fillCUE(metadata, info.tracks, info.cueFilePath);

        return;
    }

    QStringList filePaths;
    filePaths.reserve(MINIMUM_TRACK_COUNT);

    const auto appendIfExists =
        [this, &dirPath, &filePaths](const QString& relativePath) -> void {
        const QString localFilePath = dirPath + relativePath;

        if (QFile::exists(localFilePath)) {
            filePaths.append(localFilePath);
        }
    };

    if (extension == EXT_XSPF) {
        QXmlStreamReader xml = QXmlStreamReader(&file);

        while (!xml.atEnd()) {
            if (xml.readNext() == QXmlStreamReader::StartElement &&
                xml.name() == u"location") {
                appendIfExists(xml.readElementText());
            }
        }

        if (xml.hasError()) {
            QMessageBox::warning(
                this,
                tr("Error occurred when parsing XSPF"),
                xml.errorString()
            );
            return;
        }
    } else {
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

    playlistView->table(index)->fill(filePaths);
}

void MainWindow::exportPlaylist(const PlaylistFileType playlistType) {
    const i8 index = playlistView->currentIndex();
    if (index == -1) {
        return;
    }

    const TrackTable* const table = playlistView->table(index);

    QString outputPath =
        QFileDialog::getExistingDirectory(this, tr("Select Output Directory"));

    if (outputPath.isEmpty()) {
        return;
    }

    const auto tabLabel = playlistView->tabLabel(index);
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

    const i32 rowCount = table->model()->rowCount();
    vector<TrackMetadata> properties;
    properties.reserve(rowCount);

    for (const i32 row : range(0, rowCount)) {
        properties.emplace_back(table->rowMetadata(row));
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

void MainWindow::adjustPlaylistView() {
    DockWidget::Position dockWidgetPosition;
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

    if (mainAreaOrientation == Qt::Horizontal) {
        dockWidgetPosition = dockWidgetIndex == 0 ? DockWidget::Position::Left
                                                  : DockWidget::Position::Right;
    } else {
        dockWidgetPosition = dockWidgetIndex == 0
                                 ? DockWidget::Position::Top
                                 : DockWidget::Position::Bottom;
    }

    const i8 currentIndex = playlistView->currentIndex();

    if (currentIndex == -1) {
        return;
    }

    if (!searchTrackInput->isHidden()) {
        toggleSearchInput(true);
    }

    adjustPlaylistTabBar(dockWidgetPosition, currentIndex);

    if (!playlistView->hasBackgroundImage(currentIndex)) {
        return;
    }

    adjustPlaylistImage(dockWidgetPosition, currentIndex);
}

void MainWindow::adjustPlaylistTabBar(
    const DockWidget::Position dockWidgetPosition,
    const u8 currentIndex
) {
    switch (dockWidgetPosition) {
        case DockWidget::Position::Right: {
            const i32 dockWidgetX = dockWidget->x();

            if (dockWidgetX == -1) {
                playlistTabBar->setScrollAreaWidth(mainArea->width());
            } else {
                playlistTabBar->setScrollAreaWidth(dockWidgetX);
            }
            break;
        }
        case DockWidget::Position::Left:
        case DockWidget::Position::Top:
        case DockWidget::Position::Bottom:
            playlistTabBar->setScrollAreaWidth(mainArea->width());
            break;
    }
}

void MainWindow::adjustPlaylistImage(
    const DockWidget::Position dockWidgetPosition,
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

    const QWidget* const pageWidget = playlistView->page(currentIndex);
    auto* const leftLabel = pageWidget->findChild<QLabel*>(u"leftLabel");
    auto* const centerLabel = pageWidget->findChild<QLabel*>(u"centerLabel");
    auto* const rightLabel = pageWidget->findChild<QLabel*>(u"rightLabel");

    const u16 yPos = playlistView->y();
    const u16 centerLabelHalfWidth = centerLabel->width() / 2;
    const u16 rightLabelWidth = rightLabel->width();
    const u16 halfWidth = width / 2;

    const u16 availableWidthLeft = width - dockWidgetWidth;
    const u16 halfAvailableWidthLeft = availableWidthLeft / 2;

    const u16 availableWidthRight = dockWidgetX;
    const u16 halfAvailableWidthRight = availableWidthRight / 2;

    switch (dockWidgetPosition) {
        case DockWidget::Position::Right:
            leftLabel->move(0, yPos);
            centerLabel->move(
                halfAvailableWidthRight - centerLabelHalfWidth,
                yPos
            );
            rightLabel->move(availableWidthRight - rightLabelWidth, yPos);
            break;
        case DockWidget::Position::Left:
            leftLabel->move(0, yPos);
            centerLabel->move(
                halfAvailableWidthLeft - centerLabelHalfWidth,
                yPos
            );
            rightLabel->move(availableWidthLeft - rightLabelWidth, yPos);
            break;
        case DockWidget::Position::Top:
        case DockWidget::Position::Bottom:
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

    for (const QLatin1StringView extension : SUPPORTED_PLAYABLE_EXTENSIONS) {
        filter += "*."_L1;
        filter += extension;
        filter += ' ';
    }

    filter.removeLast();  // Remove space
    filter += ')';
    return filter;
}

void MainWindow::handleConnectionIPC() {
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
}

void MainWindow::showVisualizer() {
#ifdef PROJECTM
    if (visualizerDialog != nullptr) {
        return;
    }

    visualizerDialog = new VisualizerDialog(settings);

    visualizerDialog->setChannels(audioWorker->channels());
    visualizerDialog->show();

    connect(
        audioWorker,
        &AudioWorker::audioProperties,
        visualizerDialog,
        [this](const u32 /* sampleRate */, const u8 channels) -> void {
        visualizerDialog->setChannels(channels);
    }
    );

    connect(
        visualizerDialog,
        &VisualizerDialog::rejected,
        this,
        [this] -> void {
        delete visualizerDialog;
        visualizerDialog = nullptr;
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
}

inline void MainWindow::showRepeatButtonContextMenu(const QPoint& pos) {
    if (playlistView->repeatMode() != PlaylistView::RepeatMode::Track) {
        return;
    }

    if (trackRepeatMenu->isHidden()) {
        trackRepeatMenu->move(
            repeatButton->mapToGlobal(QPoint{ 0, repeatButton->height() })
        );

        trackRepeatMenu->show();
    }
};

inline void MainWindow::showControlContainerContextMenu() {
    auto* const menu = new QMenu(this);

    bool visualizerShown = !spectrumVisualizer->isHidden();
    QAction* const toggleSpectrumVisAction =
        menu->addAction(tr("Spectrum Visualizer"));
    toggleSpectrumVisAction->setCheckable(true);
    toggleSpectrumVisAction->setChecked(visualizerShown);

    const QAction* const selectedAction = menu->exec(QCursor::pos());
    delete menu;

    if (selectedAction == toggleSpectrumVisAction) {
        spectrumVisualizer->setHidden(visualizerShown);
        visualizerShown = !visualizerShown;
        settings->spectrumVisualizer.hidden = !visualizerShown;
    }
};

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* const ui_ = new Ui::MainWindow();
    ui_->setupUi(this);
    return ui_;
};

void MainWindow::checkForUpdates(bool manual) {
    if (!settings->core.checkForUpdates && !manual) {
        return;
    }

    auto* const updater = new AutoUpdater(this);

    connect(
        updater,
        &AutoUpdater::updateDownloaded,
        this,
        [this](const QByteArray& archiveData) -> void {
        const QString appDir = qApp->applicationDirPath();

#ifdef Q_OS_WINDOWS
        const QString exePath = qApp->applicationFilePath();
        QFile::rename(exePath, appDir + u"/rap-old.exe");

        const Bit7zLibrary lib(appDir.toStdString() + "/7zxa.dll");

        vector<byte_t> archiveVec(archiveData.size());
        memcpy(archiveVec.data(), archiveData.data(), archiveData.size());

        const BitArchiveReader archive{ lib,
                                        archiveVec,
                                        ArchiveStartOffset::None,
                                        BitFormat::SevenZip };

        vector<u32> indicesToExtract;

        optional<u32> rapExeIdx;
        optional<u32> iconIdx;

        for (const auto& item : archive.items()) {
            const string name = item.name();

            if (name == "rap.exe") {
                rapExeIdx = item.index();
            } else if (name == "icons/rap-logo.ico") {
                iconIdx = item.index();
            }
        }

        if (rapExeIdx) {
            indicesToExtract.push_back(*rapExeIdx);
        }

        if (iconIdx) {
            indicesToExtract.push_back(*iconIdx);
        }

        if (!indicesToExtract.empty()) {
            archive.extractTo(appDir.toStdString(), indicesToExtract);

            if (rapExeIdx) {
                QFile::rename(
                    appDir + u"/rap/rap.exe"_qssv,
                    appDir + u"/rap.exe"_qssv
                );
            }

            if (iconIdx) {
                QDir().mkpath(appDir + u"/icons"_qssv);

                QFile::rename(
                    appDir + u"/rap/icons/rap-logo.ico"_qssv,
                    appDir + u"/icons/rap-logo.ico"_qssv
                );
            }

            QFile::remove(appDir + u"/rap.7z"_qssv);
            QDir(appDir + u"/rap"_qssv).removeRecursively();
        }
#elifdef Q_OS_LINUX
        const QString archivePath = appDir + u"/rap.tar.xz"_qssv;
        auto archiveFile = QFile(archivePath);

        if (!archiveFile.open(QFile::WriteOnly)) {
            QMessageBox::information(
                this,
                tr("Update failed"),
                tr("Failed to write archive file")
            );
            return;
        }

        archiveFile.write(archiveData);
        archiveFile.close();

        QProcess tarProcess;
        tarProcess.setWorkingDirectory(appDir);

        tarProcess.start(
            u"tar"_s,
            { u"-xf"_s, archivePath, u"rap"_s, u"icons/rap-logo.png"_s }
        );

        tarProcess.waitForFinished(5000);

        if (tarProcess.exitStatus() != QProcess::NormalExit ||
            tarProcess.exitCode() != 0) {
            QMessageBox::information(
                this,
                tr("Unarchiving update failed"),
                tr("Executing tar failed with: %1")
                    .arg(tarProcess.readAllStandardError())
            );
        } else {
            QFile::setPermissions(
                appDir + u"/rap"_qssv,
                QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                    QFileDevice::ExeOwner | QFileDevice::ReadGroup |
                    QFileDevice::ExeGroup | QFileDevice::ReadOther |
                    QFileDevice::ExeOther
            );
        }

        QFile::remove(archivePath);
#elifdef Q_OS_MAC
// TODO: implement this some day
#endif

        qApp->quit();
        QProcess::startDetached(qApp->arguments()[0]);
    },
        Qt::SingleShotConnection
    );

    connect(
        updater,
        &AutoUpdater::versionFetched,
        this,
        [=, this](const QString& version) -> void {
        const auto newVersion = QVersionNumber::fromString(version);
        const auto currentVersion = QVersionNumber::fromString(APP_VERSION);

        if (newVersion <= currentVersion) {
            if (manual) {
                QMessageBox::information(
                    this,
                    tr("Up to date"),
                    tr("Program is up-to-date.")
                );
            } else {
                qInfo() << "Up to date."_L1;
            }

            return;
        }

        auto msgBox = QMessageBox(this);
        auto* const checkbox = new QCheckBox(tr("Don't remind me"), &msgBox);
        msgBox.setWindowTitle(tr("New version is available"));
        msgBox.setText(tr("Version %1 is available.\nCurrent version is %2.")
                           .arg(version)
                           .arg(APP_VERSION));
        const QPushButton* const installButton =
            msgBox.addButton(tr("Install"), QMessageBox::AcceptRole);
        const QPushButton* const skipButton =
            msgBox.addButton(tr("Skip"), QMessageBox::RejectRole);
        msgBox.setCheckBox(checkbox);

        msgBox.exec();
        const auto* const clickedButton = msgBox.clickedButton();

        if (clickedButton == installButton) {
            updater->downloadUpdate();

            updateProgressDialog = new QProgressDialog(
                tr("Installing update..."),
                tr("Abort"),
                0,
                1,
                this
            );
            updateProgressDialog->setWindowModality(Qt::WindowModal);

            connect(
                updateProgressDialog,
                &QProgressDialog::canceled,
                this,
                [updater] -> void { updater->abortDownload(); }
            );

            connect(
                updater,
                &AutoUpdater::updateDownloadProgress,
                this,
                [this](const u64 received, const u64 total) -> void {
                updateProgressDialog->setMaximum(i32(total));
                updateProgressDialog->setValue(i32(received));
            }
            );
        } else if (clickedButton == skipButton) {
            if (checkbox->isChecked()) {
                settings->core.checkForUpdates = false;
            }

            updater->deleteLater();
        }
    },
        Qt::SingleShotConnection
    );

    connect(
        updater,
        &AutoUpdater::updateFailed,
        this,
        [=, this](const QNetworkReply::NetworkError error) -> void {
        QMessageBox::warning(
            this,
            tr("Update failed"),
            tr("Update failed with error: %1").arg(error)
        );

        updater->deleteLater();
    }
    );

    updater->checkForUpdates();
}

void MainWindow::toggleRepeat() {
    switch (playlistView->repeatMode()) {
        case PlaylistView::RepeatMode::Off:
            trackRepeatMenu->hide();
            repeatButton->setChecked(true);
            actionRepeat->setText(tr("Repeat (Playlist)"));
            break;
        case PlaylistView::RepeatMode::Playlist:
            repeatButton->setChecked(true);
            repeatButton->setText(u"1"_s);
            actionRepeat->setText(tr("Repeat (Track)"));
            actionRepeat->setChecked(true);
            break;
        case PlaylistView::RepeatMode::Track:
            trackRepeatMenu->hide();
            repeatButton->setChecked(false);
            repeatButton->setText(QString());
            actionRepeat->setText(tr("Repeat"));
            break;
        default:
            break;
    }

    playlistView->toggleRepeatMode();
}
