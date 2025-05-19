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
#include "helpwindow.hpp"
#include "metadatawindow.hpp"
#include "musicheader.hpp"
#include "playlistview.hpp"
#include "randint.hpp"
#include "settings.hpp"
#include "settingswindow.hpp"
#include "tominutes.hpp"
#include "trackproperties.hpp"
#include "tracktree.hpp"

#include <QDirIterator>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QWidgetAction>
#include <QXmlStreamReader>

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* ui_ = new Ui::MainWindow();
    ui_->setupUi(this);
    return ui_;
}

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
        &MainWindow::updateProgressLabel
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
        &MainWindow::updateProgressLabel
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
        audioWorker,
        &AudioWorker::duration,
        this,
        &MainWindow::updateProgress
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
        exportPlaylist(M3U);
    });

    connect(actionExportXSPFPlaylist, &QAction::triggered, this, [&] {
        exportPlaylist(XSPF);
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
        [&](const TabRemoveMode mode, const u8 index, const u8 count)  // NOLINT
    {
        if (playingPlaylist >= index) {
            playingPlaylist = as<i8>(playingPlaylist - count);
            changePlaylist(playingPlaylist);
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
                settings->outputDevice
            );
        }

        previousDefaultAudioDevice = QMediaDevices::defaultAudioOutput();
    }
    );

    connect(dockWidget, &DockWidget::resized, this, [&] {
        DockWidgetPosition dockWidgetPosition;
        const Qt::Orientation mainAreaOrientation = mainArea->orientation();
        const u8 dockWidgetIndex = mainArea->indexOf(dockWidget);

        if (mainAreaOrientation == Qt::Horizontal) {
            dockWidgetPosition = dockWidgetIndex == 0 ? Left : Right;
        } else {
            dockWidgetPosition = dockWidgetIndex == 0 ? Top : Bottom;
        }

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

        u16 dockWidgetWidth = dockWidget->width();
        u16 dockWidgetHeight = dockWidget->height();

        const i8 currentIndex = playlistView->currentIndex();
        if (currentIndex == -1) {
            return;
        }

        if (!playlistView->hasBackgroundImage(currentIndex)) {
            return;
        }

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
            case Right:
                leftLabel->move(0, yPos);
                centerLabel->move(
                    halfAvailableWidthRight - centerLabelHalfWidth,
                    yPos
                );
                rightLabel->move(availableWidthRight - rightLabelWidth, yPos);
                break;
            case Left:
                leftLabel->move(0, yPos);
                centerLabel->move(
                    halfAvailableWidthLeft - centerLabelHalfWidth,
                    yPos
                );
                rightLabel->move(availableWidthLeft - rightLabelWidth, yPos);
                break;
            case Top:
            case Bottom:
                leftLabel->move(0, 0);
                centerLabel->move(halfWidth - centerLabelHalfWidth, 0);
                rightLabel->move(width - rightLabelWidth, 0);
                break;
            default:
                break;
        }
    });

    server->listen("revolutionary-audio-player-server");

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
        tree->fillTable(args);
        changePlaylist(as<i8>(index));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    auto* tree = trackTree;

    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        QStringList paths;

        for (const QUrl& url : urls) {
            const QString path = url.toLocalFile();

            if (url.isEmpty()) {
                continue;
            }

            const QFileInfo info(path);

            if (info.isDir()) {
                paths.append(path);
                continue;
            }

            for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
                if (path.endsWith(extension)) {
                    paths.append(path);
                    break;
                }
            }
        }

        if (paths.empty()) {
            return;
        }

        u8 index = playingPlaylist;

        if (tree == nullptr ||
            settings->flags.dragNDropMode == DragDropMode::CreateNewPlaylist) {
            index = playlistView->addTab(
                settings->flags.playlistNaming == DirectoryName
                    ? QFileInfo(paths[0]).dir().dirName()
                    : QString()
            );
            tree = playlistView->tree(index);
        }

        tree->fillTable(paths);
    }
}

void MainWindow::updatePlaybackPosition() {
    const u16 second = progressSlider->value();
    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::seekSecond,
        Qt::QueuedConnection,
        second
    );

    trackDuration =
        u"%1/%2"_s.arg(toMinutes(second / SECOND_MS))
            .arg(trackDuration.slice(trackDuration.indexOf('/') + 1));

    progressLabel->setText(trackDuration);
    progressLabelCloned->setText(trackDuration);
}

auto MainWindow::saveSettings() -> result<bool, QString> {
    if (!settingsFile.open(QIODevice::WriteOnly)) {
        return err(settingsFile.errorString());
    }

    const u8 tabCount = playlistView->tabCount();

    settings->currentTab = playlistView->currentIndex();
    settings->volume = volumeSlider->value();
    settings->tabs = vector<TabObject>(tabCount);

    for (const u8 tab : range(0, tabCount)) {
        auto* tree = playlistView->tree(tab);
        auto* model = tree->model();

        const u16 rowCount = model->rowCount();

        TabObject& tabObject = settings->tabs[tab];
        tabObject.label = playlistView->tabLabel(tab);
        tabObject.backgroundImagePath = playlistView->backgroundImagePath(tab);
        tabObject.tracks = QStringList(rowCount);

        for (const u16 row : range(0, rowCount)) {
            tabObject.tracks[row] = tree->rowMetadata(row)[Path];
        }

        for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
            tabObject.columnProperties[column] = as<TrackProperty>(
                model->headerData(column, Qt::Horizontal, PROPERTY_ROLE)
                    .toUInt()
            );
            tabObject.columnStates[column] = tree->isColumnHidden(column);
        }
    }

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

    settings->save(settingsFile);
    return true;
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

    const QString formattedVolume = u"%1%"_s.arg(settings->volume);
    volumeLabel->setText(formattedVolume);
    volumeLabelCloned->setText(formattedVolume);

    if (!settings->tabs.empty()) {
        for (const auto& tabObject : settings->tabs) {
            const u8 index = playlistView->addTab(
                tabObject.label,
                tabObject.columnProperties,
                tabObject.columnStates
            );

            auto* tree = playlistView->tree(index);
            tree->fillTable(tabObject.tracks);

            if (!tabObject.backgroundImagePath.isEmpty()) {
                QTimer::singleShot(SECOND_MS, [=, this] {
                    playlistView->setBackgroundImage(
                        index,
                        mainArea->height(),
                        tabObject.backgroundImagePath
                    );
                });
            }
        }
    }

    playlistView->setCurrentIndex(settings->currentTab);
    equalizerMenu->loadSettings();

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::changeAudioDevice,
        Qt::QueuedConnection,
        settings->outputDevice
    );

    const DockWidgetPosition dockWidgetPosition =
        settings->dockWidgetSettings.position;

    moveDockWidget(dockWidgetPosition);

    mainArea->setSizes({ QWIDGETSIZE_MAX, settings->dockWidgetSettings.size });

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

                auto* coverWindow =
                    new CoverWindow(metadata[Path], metadata[Title]);
                coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                coverWindow->show();
            } else if (selectedAction == removeSelectionAction) {
                selectedRows.removeOne(trackTree->currentIndex());

                const QModelIndex& startRow = selectedRows.first();
                const isize count = selectedRows.size();

                trackTreeModel->removeRows(startRow.row(), as<i32>(count));

                trackTree->setCurrentIndex(
                    index.row() > currentIndex.row()
                        ? currentIndex
                        : trackTreeModel->index(
                              as<i32>(currentIndex.row() - selectedRows.size()),
                              0
                          )
                );
            } else if (selectedAction == setPlaylistBackground) {
                const QString file = QFileDialog::getOpenFileName(
                    this,
                    tr("Select File"),
                    QString(),
                    tr("Image Files (*.png *.jpg *.jpeg)")
                );

                if (file.isEmpty()) {
                    return;
                }

                playlistView->setBackgroundImage(
                    playlistView->currentIndex(),
                    mainArea->height(),
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

void MainWindow::handleHeaderPress(
    const u8 index,
    const Qt::MouseButton button
) {
    if (button == Qt::RightButton) {
        auto* menu = new OptionMenu(this);

        for (const auto& [idx, label] :
             views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
            auto* action = new QAction(label, menu);
            action->setCheckable(true);

            i8 columnIndex = -1;

            for (const u8 column : range(1, TRACK_PROPERTY_COUNT)) {
                if (trackTreeModel
                        ->headerData(column, Qt::Orientation::Horizontal)
                        .toString() == label) {
                    columnIndex = as<i8>(column);
                    break;
                }
            }

            if (columnIndex != -1) {
                action->setChecked(!trackTree->isColumnHidden(columnIndex));
            } else {
                action->setEnabled(false);
            }

            connect(action, &QAction::triggered, menu, [=, this] {
                if (columnIndex != -1) {
                    const bool currentlyHidden =
                        trackTree->isColumnHidden(columnIndex);
                    trackTree->setColumnHidden(columnIndex, !currentlyHidden);

                    if (currentlyHidden) {
                        trackTree->resizeColumnToContents(columnIndex);
                    }
                }
            });

            menu->addAction(action);
        }

        menu->exec(QCursor::pos());
        menu->deleteLater();
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

            const auto iter =
                ranges::find_if(SEARCH_PROPERTIES, [&](const QStringView str) {
                return prefix == str;
            });

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
                fieldValue = metadata[Title];
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
}

void MainWindow::showSearchInput() {
    searchTrackInput->move(trackTree->mapToGlobal(
        QPoint(trackTree->width() - searchTrackInput->width(), 0)
    ));
    searchTrackInput->raise();
    searchTrackInput->show();
    searchTrackInput->setFocus();
}

void MainWindow::onAudioProgressUpdated(const u16 second) {
    if (!progressSlider->isSliderDown() &&
        !progressSliderCloned->isSliderDown()) {
        trackDuration = u"%1/%2"_s.arg(
            toMinutes(second),
            trackDuration.slice(trackDuration.indexOf('/') + 1)
        );
        progressLabel->setText(trackDuration);
        progressLabelCloned->setText(trackDuration);
        progressSlider->setValue(second);
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

void MainWindow::updateProgress(const u16 seconds) {
    trackDuration = u"0:00/%1"_s.arg(toMinutes(seconds));
    progressSlider->setRange(0, seconds);
    progressSliderCloned->setRange(0, seconds);
    progressLabel->setText(trackDuration);
    progressLabelCloned->setText(trackDuration);
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
            device
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
            tr(
                "Audio/Video Files (*.mp3 *.flac *.opus *.aac *.wav *.ogg *.m4a *.mp4 *.mkv)"
            )
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

void MainWindow::updateProgressLabel() {
    const QString progress = u"%1/%2"_s.arg(
        toMinutes(progressSlider->value()),
        trackDuration.slice(trackDuration.indexOf('/') + 1)
    );
    progressLabel->setText(progress);
    progressLabelCloned->setText(progress);
}

void MainWindow::updateVolume(const u16 value) {
    const QString formattedVolume = u"%1%"_s.arg(value);
    volumeLabel->setText(formattedVolume);
    volumeLabelCloned->setText(formattedVolume);
    audioWorker->setVolume(as<f32>(value) / 100.0F);
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
        equalizerMenu->show();
    } else {
        equalizerMenu->hide();
    }
}

void MainWindow::toggleRepeat() {
    switch (repeat) {
        case RepeatMode::Off:
            repeatButton->setChecked(true);
            repeat = RepeatMode::Playlist;
            actionRepeat->setText(tr("Repeat (Playlist)"));
            break;
        case RepeatMode::Playlist:
            repeatButton->setChecked(true);
            repeatButton->setText(u"1"_s);
            actionRepeat->setText(tr("Repeat (Track)"));
            actionRepeat->setChecked(true);
            repeat = RepeatMode::Track;
            break;
        case RepeatMode::Track:
            repeatButton->setChecked(false);
            repeatButton->setText(QString());
            actionRepeat->setText(tr("Repeat"));
            repeat = RepeatMode::Off;
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
        trackTreeHeader,
        &MusicHeader::headerPressed,
        this,
        &MainWindow::handleHeaderPress,
        Qt::UniqueConnection
    );

    connect(
        trackTreeHeader,
        &MusicHeader::sortIndicatorChanged,
        this,
        &MainWindow::resetSorting,
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

        playingPlaylist = playlistView->currentIndex();
        playTrack(trackTree, trackTreeModel->index(newRow, 0));
    }

    trackTree->clearSelection();
}

void MainWindow::playTrack(TrackTree* tree, const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const HashMap<TrackProperty, QString> metadata =
        tree->rowMetadata(index.row());
    togglePlayback(metadata[Path]);

    tree->model()->itemFromIndex(index)->setData(startIcon, Qt::DecorationRole);

    const QString artistAndTrack =
        u"%1 - %2"_s.arg(metadata[Artist], metadata[Title]);
    trackLabel->setText(artistAndTrack);
    setWindowTitle(u"RAP: %1"_s.arg(artistAndTrack));

    for (const TrackProperty property :
         views::drop(DEFAULT_COLUMN_PROPERTIES, 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(property - 1, 0))
            ->setText(1, metadata[property]);
    }

    const vector<u8> coverBytes = extractCover(metadata[Path].toUtf8().data());

    QPixmap cover;
    cover.loadFromData(coverBytes.data(), coverBytes.size());
    dockCoverLabel->setPixmap(cover);
    dockCoverLabel->setMinimumSize(MINIMUM_COVER_SIZE);
}

void MainWindow::togglePlayback(const QString& path) {
    if (!path.isEmpty()) {
        QMetaObject::invokeMethod(
            audioWorker,
            &AudioWorker::start,
            Qt::QueuedConnection,
            path
        );
        playButton->setIcon(pauseIcon);
        playButton->setToolTip(tr("Pause"));
        actionTogglePlayback->setText(tr("Pause"));
    } else {
        const QAudio::State state = audioWorker->state();

        if (state == QAudio::ActiveState) {
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::suspend,
                Qt::QueuedConnection
            );
            playButton->setIcon(startIcon);
            playButton->setToolTip(tr("Play"));
            actionTogglePlayback->setText(tr("Play"));
        } else if (state != QAudio::IdleState &&
                   state != QAudio::StoppedState) {
            QMetaObject::invokeMethod(
                audioWorker,
                &AudioWorker::resume,
                Qt::QueuedConnection
            );
            playButton->setIcon(pauseIcon);
            playButton->setToolTip(tr("Pause"));
            actionTogglePlayback->setText(tr("Pause"));
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
                    case Playlist:
                        nextRow = 0;
                        break;
                    case Off: {
                        const bool success =
                            changePlaylist(as<i8>(playingPlaylist + 1));

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

    audioWorker->stop();
    trackLabel->clear();
    progressSlider->setRange(0, 0);
    progressSliderCloned->setRange(0, 0);
    progressLabel->setText(ZERO_DURATION);
    progressLabelCloned->setText(ZERO_DURATION);
    trackDuration = ZERO_DURATION;
    playButton->setIcon(startIcon);
    actionTogglePlayback->setText(tr("Play"));
    setWindowTitle("RAP");

    dockCoverLabel->clear();

    for (const TrackProperty property :
         views::drop(DEFAULT_COLUMN_PROPERTIES, 1)) {
        dockMetadataTree
            ->itemFromIndex(dockMetadataTree->model()->index(property - 1, 0))
            ->setText(1, QString());
    }
}

void MainWindow::resetSorting(
    const i32 /* unused */,
    const Qt::SortOrder sortOrder
) {
    if (trackTreeHeader->sortIndicatorSection() == -1) {
        trackTree->sortByPath();
    }
};

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

    const QFileInfo filePathInfo = QFileInfo(filePath);
    const QString dirPath = filePathInfo.absolutePath() + '/';
    const QString extension = filePathInfo.suffix().toLower();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), file.errorString());
        return;
    }

    QStringList filePaths;
    filePaths.reserve(MINIMUM_TRACK_COUNT);

    const auto appendIfExists = [&](const QString& relativePath) {
        const QString localFilePath = dirPath + relativePath;

        if (QFile::exists(localFilePath)) {
            filePaths.append(localFilePath);
        }
    };

    if (extension == "xspf") {
        QXmlStreamReader xml(&file);

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
    } else if (extension == "m3u" || extension == "m3u8") {
        QTextStream input(&file);

        while (!input.atEnd()) {
            const QString line = input.readLine().trimmed();

            if (!line.isEmpty() && !line.startsWith('#')) {
                appendIfExists(line);
            }
        }
    } else if (extension == "cue") {
        // TODO
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

    outputPath += u"/%1.%2"_s.arg(
        playlistView->tabLabel(index),
        playlistType == XSPF ? "xspf" : "m3u8"
    );

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

    const auto result = playlistType == XSPF
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

static constexpr auto getXSPFTag(TrackProperty property) -> QString {
    switch (property) {
        case TrackProperty::Title:
            return "title";
        case TrackProperty::Artist:
            return "creator";
        case TrackProperty::Album:
            return "album";
        case TrackProperty::AlbumArtist:
            return "albumArtist";
        case TrackProperty::Genre:
            return "genre";
        case TrackProperty::Duration:
            return "duration";
        case TrackProperty::TrackNumber:
            return "trackNum";
        case TrackProperty::Comment:
            return "annotation";
        case TrackProperty::Path:
            return "location";
        case TrackProperty::Composer:
            return "composer";
        case TrackProperty::Publisher:
            return "publisher";
        case TrackProperty::Year:
            return "year";
        case TrackProperty::BPM:
            return "bpm";
        case TrackProperty::Language:
            return "language";
        case TrackProperty::DiscNumber:
            return "disc";
        case TrackProperty::Bitrate:
            return "bitrate";
        case TrackProperty::SampleRate:
            return "samplerate";
        case TrackProperty::Channels:
            return "channels";
        case TrackProperty::Format:
            return "format";
        default:
            return {};
            break;
    }
}

auto MainWindow::exportXSPF(
    const QString& outputPath,
    const vector<HashMap<TrackProperty, QString>>& metadataVector
) -> result<bool, QString> {
    QFile file(outputPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return err(file.errorString());
    }

    QTextStream output(&file);
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
            const QString tag = getXSPFTag(property);

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

    QTextStream output(&outputFile);
    output.setEncoding(QStringConverter::Utf8);

    output << "#EXTM3U";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        const QString title = metadata.find(Title)->second;
        const QString artist = metadata.find(Artist)->second;
        const QString path = metadata.find(Path)->second;
        const QString durationStr = metadata.find(Duration)->second;

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
