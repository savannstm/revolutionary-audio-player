#include "mainwindow.hpp"

#include "aboutwindow.hpp"
#include "aliases.hpp"
#include "audiostreamer.hpp"
#include "audioworker.hpp"
#include "coverwindow.hpp"
#include "customslider.hpp"
#include "enums.hpp"
#include "equalizermenu.hpp"
#include "extractmetadata.hpp"
#include "helpwindow.hpp"
#include "metadatawindow.hpp"
#include "musicheader.hpp"
#include "musicitem.hpp"
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
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <QWidgetAction>
#include <qnamespace.h>
#include <qsplitter.h>

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* ui_ = new Ui::MainWindow();
    ui_->setupUi(this);
    return ui_;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    connect(
        equalizerButton,
        &QPushButton::toggled,
        this,
        &MainWindow::toggleEqualizerMenu
    );

    connect(actionForward, &QAction::triggered, this, &MainWindow::jumpForward);

    connect(
        actionBackward,
        &QAction::triggered,
        this,
        &MainWindow::jumpBackward
    );

    connect(actionRepeat, &QAction::triggered, this, &MainWindow::toggleRepeat);

    connect(actionPause, &QAction::triggered, this, &MainWindow::pausePlayback);

    connect(
        actionResume,
        &QAction::triggered,
        this,
        &MainWindow::resumePlayback
    );

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

    connect(actionHelp, &QAction::triggered, this, &MainWindow::showHelpWindow);

    connect(
        playlistView,
        &PlaylistView::indexChanged,
        this,
        &MainWindow::changePlaylist
    );

    connect(playlistView, &PlaylistView::tabRemoved, this, [&](const i8 index) {
        if (index > playingPlaylist) {
            playingPlaylist--;
        }
    });

    connect(actionRussian, &QAction::triggered, this, [&] {
        retranslate(QLocale::Russian);
    });

    connect(actionEnglish, &QAction::triggered, this, [&] {
        retranslate(QLocale::English);
    });

    connect(
        playButton,
        &QPushButton::pressed,
        this,
        &MainWindow::togglePlayback
    );

    connect(dockWidget, &QSplitter::customContextMenuRequested, this, [&] {
        auto* menu = new QMenu(this);

        const QAction* moveToLeftAction = menu->addAction(tr("Move To Left"));
        const QAction* moveToRightAction = menu->addAction(tr("Move To Right"));
        const QAction* moveToTopAction = menu->addAction(tr("Move To Top"));
        const QAction* moveToBottomAction =
            menu->addAction(tr("Move To Bottom"));

        const bool dockCoverLabelHidden = dockCoverLabel->isHidden();
        QAction* showCoverAction = menu->addAction(tr("Show Cover"));
        showCoverAction->setCheckable(true);
        showCoverAction->setChecked(dockCoverLabelHidden);

        const bool dockMetadataTreeHidden = dockMetadataTree->isHidden();
        QAction* showMetadataAction = menu->addAction(tr("Show Metadata"));
        showMetadataAction->setCheckable(true);
        showMetadataAction->setChecked(dockMetadataTreeHidden);

        const QAction* selectedAction = menu->exec(QCursor::pos());
        delete menu;

        if (selectedAction == moveToLeftAction) {
            moveDockWidget(Left);
        } else if (selectedAction == moveToRightAction) {
            moveDockWidget(Right);
        } else if (selectedAction == moveToTopAction) {
            moveDockWidget(Top);
        } else if (selectedAction == moveToBottomAction) {
            moveDockWidget(Bottom);
        } else if (selectedAction == showCoverAction) {
            if (dockCoverLabelHidden) {
                dockCoverLabel->show();
            } else if (!dockMetadataTreeHidden) {
                dockCoverLabel->hide();
            }
        } else if (selectedAction == showMetadataAction) {
            if (dockMetadataTreeHidden) {
                dockMetadataTree->show();
            } else if (!dockCoverLabelHidden) {
                dockMetadataTree->hide();
            }
        }
    });

    progressLabelCloned->setText(trackDuration);
    progressSliderCloned->setRange(0, 0);
    volumeSliderCloned->setRange(0, MAX_VOLUME);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

    setupTrayIcon();

    stopButton->setAction(actionStop);
    backwardButton->setAction(actionBackward);
    forwardButton->setAction(actionForward);
    repeatButton->setAction(actionRepeat);
    randomButton->setAction(actionRandom);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText(tr("Track name/property"));
    searchTrackInput->setMinimumWidth(MINIMUM_MATCHES_NUMBER);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    equalizerMenu = new EqualizerMenu(equalizerButton, audioWorker);
    loadSettings();
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
    hide();
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    auto* tree = trackTree;

    if (tree == nullptr ||
        settings->flags.dragNDropMode == DragDropMode::CreateNewPlaylist) {
        const i8 index = playlistView->addTab(QString());
        tree = playlistView->tree(index);
    }

    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (!url.isEmpty()) {
                tree->processFile(url.toLocalFile());
            }
        }
    }

    for (u8 column = TRACK_PROPERTY_COUNT - 1; column >= 0; column--) {
        if (tree->model()->trackProperty(column) == Path) {
            tree->sortByColumn(column, Qt::AscendingOrder);
            break;
        }
    }
}

void MainWindow::updatePlaybackPosition() {
    if (audioWorker->state() == QtAudio::IdleState) {
        return;
    }

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

void MainWindow::jumpToTrack(const Direction direction, const bool clicked) {
    auto* trackTree = playlistView->tree(playingPlaylist);

    if (trackTree == nullptr) {
        stopPlayback();
        return;
    }

    auto* trackTreeModel = trackTree->model();
    auto* trackTreeHeader = trackTree->header();

    const u16 rowCount = trackTreeModel->rowCount();
    if (rowCount == 0) {
        return;
    }

    const QModelIndex currentIndex = trackTree->currentIndex();
    trackTreeModel->item(currentIndex.row(), 0)->setText(QString());

    const u16 currentRow = currentIndex.row();
    u16 nextRow;

    switch (direction) {
        case Direction::Forward:
            if (currentRow == rowCount - 1) {
                if (repeat == RepeatMode::Playlist || clicked) {
                    nextRow = 0;
                } else {
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
            if (currentRow == 0) {
                nextRow = rowCount - 1;
            } else {
                nextRow = currentRow - 1;
            }
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

    const QModelIndex index = trackTreeModel->index(nextRow, 0);
    trackTree->setCurrentIndex(index);

    playTrack(trackTree, index);
}

void MainWindow::stopPlayback() {
    if (trackTree != nullptr) {
        trackTree->clearSelection();
        trackTreeModel->item(trackTree->currentIndex().row(), 0)
            ->setText(QString());
        trackTree->setCurrentIndex(trackTreeModel->index(-1, -1));
    }

    audioWorker->stop();
    trackLabel->clear();
    progressSlider->setRange(0, 0);
    progressSliderCloned->setRange(0, 0);
    progressLabel->setText(ZERO_DURATION);
    progressLabelCloned->setText(ZERO_DURATION);
    trackDuration = ZERO_DURATION;
    playButton->setIcon(startIcon);
    setWindowTitle("RAP");

    dockCoverLabel->clear();
    for (u8 i = 0; i < TRACK_PROPERTY_COUNT; i++) {
        auto* item = dockMetadataTree->itemFromIndex(
            dockMetadataTree->model()->index(as<i32>(i), 0)
        );

        if (item == nullptr) {
            continue;
        }

        item->setText(0, QString());
        item->setText(1, QString());
    }
}

void MainWindow::playTrack(TrackTree* tree, const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const u16 row = index.row();
    const MetadataMap metadata = tree->rowMetadata(row);

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::start,
        Qt::QueuedConnection,
        metadata[Path]
    );

    tree->model()->itemFromIndex(index)->setData(startIcon, Qt::DecorationRole);

    const QString artistAndTrack =
        u"%1 - %2"_s.arg(metadata[Artist], metadata[Title]);

    trackLabel->setText(artistAndTrack);
    setWindowTitle(u"RAP: %1"_s.arg(artistAndTrack));

    for (const auto& [idx, label] :
         views::drop(views::enumerate(trackPropertiesLabels()), 1)) {
        qDebug() << idx;
        auto* item = dockMetadataTree->itemFromIndex(
            dockMetadataTree->model()->index(as<i32>(idx - 1), 0)
        );

        if (item == nullptr) {
            continue;
        }

        item->setText(0, label);
        item->setText(1, metadata[idx]);
    }

    QPixmap cover;
    const vector<u8> coverBytes = extractCover(metadata[Path].toUtf8().data());
    cover.loadFromData(coverBytes.data(), coverBytes.size());
    dockCoverLabel->setPixmap(cover);
    dockCoverLabel->setMinimumSize(MINIMUM_COVER_SIZE);
}

auto MainWindow::saveSettings() -> result<bool, QString> {
    if (!settingsFile.open(QIODevice::WriteOnly)) {
        return err(settingsFile.errorString());
    }

    const i8 tabCount = as<i8>(playlistView->tabCount() - 1);

    settings->currentTab = playlistView->currentIndex();
    settings->volume = volumeSlider->value();
    settings->tabs = vector<TabObject>(tabCount);

    for (i8 i = 0; i < tabCount; i++) {
        auto* tree = playlistView->tree(i);
        auto* model = tree->model();

        const u16 rowCount = model->rowCount();

        TabObject& tabObject = settings->tabs[i];
        tabObject.label = playlistView->tabLabel(i);
        tabObject.backgroundImagePath = playlistView->backgroundImagePath(i);
        tabObject.tracks = QStringList(rowCount);

        for (u16 row = 0; row < rowCount; row++) {
            tabObject.tracks[row] = tree->rowMetadata(row)[Path];
        }

        for (u8 column = 0; column < TRACK_PROPERTY_COUNT; column++) {
            tabObject.columnProperties[column] = as<TrackProperty>(
                model->headerData(column, Qt::Horizontal, PROPERTY_ROLE)
                    .toUInt()
            );
            tabObject.columnStates[column] = tree->isColumnHidden(column);
        }
    }

    const EqualizerInfo equalizerInfo = equalizerMenu->equalizerInfo();
    settings->equalizerSettings.enabled = equalizerInfo.enabled;
    settings->equalizerSettings.bandIndex = equalizerInfo.bandIndex;
    settings->equalizerSettings.gains = equalizerInfo.gains;
    settings->equalizerSettings.frequencies = equalizerInfo.frequencies;

    DockWidgetPosition dockWidgetPosition;
    Qt::Orientation mainAreaOrientation = mainArea->orientation();
    u8 dockWidgetIndex = mainArea->children().indexOf(dockMetadataTree);

    if (mainAreaOrientation == Qt::Horizontal) {
        if (dockWidgetIndex == 0) {
            dockWidgetPosition = Left;
        } else {
            dockWidgetPosition = Right;
        }
    } else {
        if (dockWidgetIndex == 0) {
            dockWidgetPosition = Top;
        } else {
            dockWidgetPosition = Bottom;
        }
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

    const QStringList propertyLabelMap = trackPropertiesLabels();

    for (i8 tab = 0; tab < playlistView->tabCount() - 1; tab++) {
        auto* tree = playlistView->tree(tab);
        auto* model = tree->model();

        const QString label = playlistView->tabLabel(tab);
        const QString number = label.split(' ').last();

        bool numberIsNumber = true;

        for (const QChar chr : number) {
            if (!chr.isDigit()) {
                numberIsNumber = false;
                break;
            }
        }

        playlistView->setTabLabel(
            tab,
            numberIsNumber ? tr("Playlist %1").arg(number) : label
        );

        for (u8 column = 0; column < model->columnCount(); column++) {
            model->setHeaderData(
                column,
                Qt::Horizontal,
                propertyLabelMap[as<TrackProperty>(column)]
            );
        }
    }

    emit retranslated();
}

void MainWindow::loadSettings() {
    if (!settingsFile.open(QIODevice::ReadOnly)) {
        settings = make_shared<Settings>();
    } else {
        const QByteArray jsonData = settingsFile.readAll();
        settingsFile.close();

        settings = make_shared<Settings>(
            QJsonDocument::fromJson(jsonData, nullptr).object()
        );
    }

    retranslate(settings->language);

    volumeSlider->setValue(settings->volume);
    volumeSliderCloned->setValue(settings->volume);

    const QString formattedVolume = u"%1%"_s.arg(settings->volume);
    volumeLabel->setText(formattedVolume);
    volumeLabelCloned->setText(formattedVolume);

    if (!settings->tabs.empty()) {
        for (const auto& tabObject : settings->tabs) {
            const i8 index = playlistView->addTab(
                tabObject.label,
                tabObject.columnProperties,
                tabObject.columnStates
            );

            auto* tree = playlistView->tree(index);
            tree->fillTable(tabObject.tracks);

            if (!tabObject.backgroundImagePath.isEmpty()) {
                // TODO: Sets incorrectly, because layout isn't loaded
                // SOLUTION: Load the first layout, zoom into it, and get
                // dimensions.
                playlistView->setBackgroundImage(
                    index,
                    tabObject.backgroundImagePath
                );
            }
        }
    }

    playlistView->setCurrentIndex(settings->currentTab);

    equalizerMenu->setEqualizerInfo(
        settings->equalizerSettings.enabled,
        settings->equalizerSettings.bandIndex,
        settings->equalizerSettings.gains,
        settings->equalizerSettings.frequencies
    );

    const DockWidgetPosition dockWidgetPosition =
        settings->dockWidgetSettings.position;

    moveDockWidget(dockWidgetPosition);

    mainArea->setSizes({ QWIDGETSIZE_MAX, settings->dockWidgetSettings.size });
}

void MainWindow::moveDockWidget(const DockWidgetPosition dockWidgetPosition) {
    const Qt::Orientation mainAreaOrientation = mainArea->orientation();
    const u8 dockWidgetOrder = mainArea->indexOf(dockWidget);

    switch (dockWidgetPosition) {
        case Left:
            if (mainAreaOrientation == Qt::Horizontal && dockWidgetOrder == 0) {
                return;
            }

            mainArea->setOrientation(Qt::Horizontal);
            mainArea->setOrientation(Qt::Vertical);
            mainArea->widget(dockWidgetOrder)->setParent(nullptr);
            mainArea->insertWidget(0, dockWidget);
            break;
        case Right:
            if (mainAreaOrientation == Qt::Horizontal && dockWidgetOrder == 1) {
                return;
            }

            mainArea->setOrientation(Qt::Horizontal);
            mainArea->setOrientation(Qt::Vertical);
            mainArea->widget(dockWidgetOrder)->setParent(nullptr);
            mainArea->insertWidget(1, dockWidget);
            break;
        case Top:
            if (mainAreaOrientation == Qt::Vertical && dockWidgetOrder == 0) {
                return;
            }

            mainArea->setOrientation(Qt::Vertical);
            dockWidget->setOrientation(Qt::Horizontal);
            mainArea->widget(dockWidgetOrder)->setParent(nullptr);
            mainArea->insertWidget(0, dockWidget);
            break;
        case Bottom:
            if (mainAreaOrientation == Qt::Vertical && dockWidgetOrder == 1) {
                return;
            }

            mainArea->setOrientation(Qt::Vertical);
            dockWidget->setOrientation(Qt::Horizontal);
            mainArea->widget(dockWidgetOrder)->setParent(nullptr);
            mainArea->insertWidget(1, dockWidget);
            break;
    }
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

            if (playlistView->hasBackgroundImage(playlistView->currentIndex()
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
                const MetadataMap metadata =
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
                    file
                );
            } else if (selectedAction == removePlaylistBackground) {
                playlistView->removeBackgroundImage(playlistView->currentIndex()
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

        for (const QString& label : trackPropertiesLabels()) {
            auto* action = new QAction(label, menu);
            action->setCheckable(true);

            i8 columnIndex = -1;

            for (u8 column = 0; column < TRACK_PROPERTY_COUNT; column++) {
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
                }
            });

            menu->addAction(action);
        }

        menu->exec(QCursor::pos());
        menu->deleteLater();
    }
}

void MainWindow::searchTrack() {
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

        for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
            const MetadataMap metadata = trackTree->rowMetadata(row);
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
}

void MainWindow::showHelpWindow() {
    auto* helpWindow = new HelpWindow(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->show();
}

void MainWindow::addEntry(const bool createNewTab, const bool isFolder) {
    QString path;
    QString searchDirectory = settings->lastOpenedDirectory.isEmpty(
                              ) || !QDir(settings->lastOpenedDirectory).exists()
                                  ? QDir::rootPath()
                                  : settings->lastOpenedDirectory;

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
            tr("Audio/Video Files (*.mp3 *.flac *.opus *.aac *.wav *.ogg *.m4a *.mp4 *.mkv)"
            )
        );

        if (path.isEmpty()) {
            return;
        }
    }

    auto* tree = trackTree;

    if (createNewTab || tree == nullptr) {
        const i8 index = playlistView->addTab(QFileInfo(path).fileName());
        tree = playlistView->tree(index);
    }

    if (isFolder) {
        QDirIterator entries(
            path,
            QDir::NoDotAndDotDot | QDir::Files,
            QDirIterator::Subdirectories
        );
        tree->fillTable(entries);
    } else {
        tree->fillTable({ path });
    }
}

void MainWindow::togglePlayback() {
    const QAudio::State state = audioWorker->state();

    if (state == QAudio::ActiveState) {
        actionPause->trigger();
    } else if (state != QAudio::IdleState) {
        actionResume->trigger();
    }
}

void MainWindow::moveForward() {
    actionForward->trigger();
}

void MainWindow::moveBackward() {
    actionBackward->trigger();
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

void MainWindow::jumpForward() {
    jumpToTrack(forwardDirection, true);
}

void MainWindow::jumpBackward() {
    jumpToTrack(backwardDirection, true);
}

void MainWindow::toggleRepeat() {
    switch (repeat) {
        case RepeatMode::Off:
            repeat = RepeatMode::Playlist;
            actionRepeat->setText(tr("Repeat (Playlist)"));
            break;
        case RepeatMode::Playlist:
            repeatButton->toggle();
            repeatButton->setText("1");
            actionRepeat->setText(tr("Repeat (Track)"));
            actionRepeat->setChecked(true);
            repeat = RepeatMode::Track;
            break;
        case RepeatMode::Track:
            repeatButton->setText(QString());
            actionRepeat->setText(tr("Repeat"));
            repeat = RepeatMode::Off;
            break;
        default:
            break;
    }
}

void MainWindow::pausePlayback() {
    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::suspend,
        Qt::QueuedConnection
    );
    playButton->setIcon(startIcon);
}

void MainWindow::resumePlayback() {
    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::resume,
        Qt::QueuedConnection
    );
    playButton->setIcon(pauseIcon);
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
        show();
        raise();
        setWindowState(windowState() & ~Qt::WindowMinimized);
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
          actionResume,
          actionPause,
          actionStop,
          actionBackward,
          actionForward,
          actionRepeat,
          actionRandom,
          actionExit }
    );

    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::changePlaylist(const i8 index) {
    if (index == -1) {
        playingPlaylist = -1;
        trackTree = nullptr;
        trackTreeHeader = nullptr;
        trackTreeModel = nullptr;
        return;
    }

    trackTree = playlistView->tree(index);
    trackTreeHeader = trackTree->header();
    trackTreeModel = trackTree->model();

    connect(
        trackTree,
        &TrackTree::trackSelected,
        this,
        &MainWindow::selectTrack
    );

    connect(
        trackTreeHeader,
        &MusicHeader::headerPressed,
        this,
        &MainWindow::handleHeaderPress
    );

    connect(
        trackTreeHeader,
        &MusicHeader::sortIndicatorChanged,
        this,
        &MainWindow::resetSorting
    );

    connect(
        trackTree,
        &TrackTree::pressed,
        this,
        &MainWindow::handleTrackPress
    );

    connect(
        trackTree->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        [=, this](
            const QItemSelection& selected,
            const QItemSelection& deselected
        ) {
        const QModelIndex requiredIndex = trackTree->currentIndex();

        if (deselected.contains(requiredIndex)) {
            trackTree->selectionModel()->select(
                requiredIndex,
                QItemSelectionModel::Select | QItemSelectionModel::Rows
            );
        }
    }
    );
}

void MainWindow::selectTrack(const i32 oldRow, const u16 newRow) {
    if (oldRow != newRow) {
        if (oldRow != -1) {
            trackTreeModel->item(oldRow, 0)->setData(
                QString(),
                Qt::DecorationRole
            );
        }

        playingPlaylist = playlistView->currentIndex();
        playTrack(trackTree, trackTreeModel->index(newRow, 0));
    }

    trackTree->clearSelection();
}

void MainWindow::resetSorting(const i32 index, const Qt::SortOrder sortOrder) {
    const i32 section = trackTreeHeader->sortIndicatorSection();

    if (section == -1) {
        for (u8 column = TRACK_PROPERTY_COUNT - 1; column >= 0; column--) {
            if (trackTreeModel->trackProperty(column) == Path) {
                trackTree->sortByColumn(column, Qt::SortOrder::AscendingOrder);
                break;
            }
        }
    }
};