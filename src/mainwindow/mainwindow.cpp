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
#include "optionmenu.hpp"
#include "playlistview.hpp"
#include "randint.hpp"
#include "settingswindow.hpp"
#include "tominutes.hpp"
#include "trackproperties.hpp"
#include "tracktree.hpp"

#include <QDirIterator>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>

constexpr u8 MINIMUM_MATCHES_NUMBER = 32;
constexpr u8 SEARCH_INPUT_MIN_WIDTH = 64;
constexpr u8 SEARCH_INPUT_HEIGHT = 24;

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* _ui = new Ui::MainWindow();
    _ui->setupUi(this);
    return _ui;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    const QString locale = QLocale::system().name();
    const QString baseName = "rap" + locale;
    if (translator.load(
            baseName,
            QApplication::applicationDirPath() + "/translations"
        )) {
        QApplication::installTranslator(&translator);
    }

    connect(
        eqButton,
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
        addFolder(true);
    });

    connect(actionOpenFile, &QAction::triggered, this, [&] { addFile(true); });

    connect(actionAddFolder, &QAction::triggered, this, [&] {
        addFolder(false);
    });

    connect(actionAddFile, &QAction::triggered, this, [&] { addFile(false); });

    connect(
        playButton,
        &QPushButton::clicked,
        this,
        &MainWindow::togglePlayback
    );

    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopPlayback);

    connect(
        forwardButton,
        &QPushButton::clicked,
        this,
        &MainWindow::moveForward
    );

    connect(
        backwardButton,
        &QPushButton::clicked,
        this,
        &MainWindow::moveBackward
    );

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
        volumeSlider,
        &CustomSlider::valueChanged,
        this,
        &MainWindow::updateVolume
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

    connect(audioWorker, &AudioWorker::endOfFile, this, &MainWindow::playNext);

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
        if (index < playingPlaylist) {
            playingPlaylist--;
        }
    });

    connect(actionRussian, &QAction::triggered, this, [&] {
        const QString baseName = "rap_ru";
        if (translator.load(
                baseName,
                QApplication::applicationDirPath() + "/translations"
            )) {
            QApplication::installTranslator(&translator);
            ui->retranslateUi(this);
        }
    });

    connect(actionEnglish, &QAction::triggered, this, [&] {
        const QString baseName = "rap_en";
        if (translator.load(
                baseName,
                QApplication::applicationDirPath() + "/translations"
            )) {
            QApplication::installTranslator(&translator);
            ui->retranslateUi(this);
        }
    });

    setAcceptDrops(true);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

    setupTrayIcon();

    repeatButton->setAction(actionRepeat);
    randomButton->setAction(actionRandom);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText("Track Name");
    searchTrackInput->setMinimumWidth(MINIMUM_MATCHES_NUMBER);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    initializeEqualizerMenu();

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
    this->hide();
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (!url.isEmpty()) {
                processFile(trackTree, url.toLocalFile());
            }
        }
    }
}

void MainWindow::processFile(TrackTree* tree, const QString& filePath) {
    for (cstr extension : ALLOWED_EXTENSIONS) {
        if (!filePath.endsWith(extension)) {
            return;
        }

        auto* model = tree->model();

        const auto metadata = extractMetadata(filePath.toStdString().c_str());

        const u16 row = model->rowCount();

        for (u16 column = 0; column < model->columnCount(); column++) {
            auto* item = new MusicItem();

            const u8 headerProperty = trackTreeModel->trackProperty(column);

            if (headerProperty == TrackNumber) {
                QString number;

                for (const auto& [idx, chr] :
                     views::enumerate(metadata[headerProperty])) {
                    if (idx == 0 && chr == '0') {
                        continue;
                    }

                    if (!chr.isDigit()) {
                        break;
                    }

                    number.append(chr);
                }

                item->setData(number.toInt(), Qt::EditRole);
            } else if (headerProperty == Play) {
                item->setText("");
            } else {
                item->setText(metadata[headerProperty]);
            }

            model->setItem(row, column, item);
        }
    }
}

inline void
MainWindow::fillTable(TrackTree* tree, const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        if (tree->model()->contains(filePath)) {
            continue;
        }

        processFile(tree, filePath);
    }

    for (u8 column = 0; column < tree->model()->columnCount(); column++) {
        tree->resizeColumnToContents(column);
    }
}

inline void MainWindow::fillTable(TrackTree* tree, QDirIterator& iterator) {
    while (iterator.hasNext()) {
        iterator.next();
        QFileInfo entry = iterator.fileInfo();

        if (!entry.isFile()) {
            continue;
        }

        const QString path = entry.filePath();

        if (tree->model()->contains(path)) {
            continue;
        }

        processFile(tree, path);
    }

    for (u8 column = 0; column < tree->model()->columnCount(); column++) {
        tree->resizeColumnToContents(column);
    }
};

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
    trackTreeModel->item(currentIndex.row(), 0)->setText("");

    u16 nextIdx;
    const u16 currentRow = currentIndex.row();

    switch (direction) {
        case Direction::Forward:
            if (currentRow == rowCount - 1) {
                if (repeat == RepeatMode::Playlist || clicked) {
                    nextIdx = 0;
                } else {
                    stopPlayback();
                    return;
                }
            } else {
                nextIdx = currentRow + 1;
            }
            break;
        case Direction::ForwardRandom: {
            playHistory.insert(currentRow);

            if (playHistory.size() == rowCount) {
                playHistory.clear();
            }

            do {
                nextIdx = randint_range(0, rowCount - 1, randdevice());
            } while (playHistory.contains(nextIdx));
            break;
        }
        case Direction::Backward:
        backward:
            if (currentRow == 0) {
                nextIdx = rowCount - 1;
            } else {
                nextIdx = currentRow - 1;
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

    qDebug() << index;
    playTrack(trackTree, index);
}

void MainWindow::stopPlayback() {
    if (trackTree != nullptr) {
        trackTree->clearSelection();
    }

    trackLabel->clear();
    setWindowTitle(u"RAP"_s);
    audioWorker->stop();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressLabel->setText(u"0:00/0:00"_s);
    playButton->setIcon(startIcon);
    trackTree->clearSelection();
}

auto MainWindow::getRowMetadata(TrackTree* tree, const u16 row)
    -> QMap<u8, QString> {
    QMap<u8, QString> result;

    auto* trackTreeModel = tree->model();
    const u8 columnCount = trackTreeModel->columnCount();

    for (u8 column = 0; column < columnCount; column++) {
        const TrackProperty headerProperty =
            trackTreeModel->trackProperty(column);

        const QModelIndex index = trackTreeModel->index(row, column);

        if (index.isValid()) {
            result[headerProperty] = trackTreeModel->data(index).toString();
        }
    }

    return result;
}

void MainWindow::playTrack(TrackTree* tree, const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const u16 row = index.row();
    const auto metadata = getRowMetadata(tree, row);

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::start,
        Qt::QueuedConnection,
        metadata[Path]
    );

    tree->model()->itemFromIndex(index)->setText("▶️");

    const QString artistAndTrack =
        u"%1 - %2"_s.arg(metadata[Artist], metadata[Title]);

    trackLabel->setText(artistAndTrack);
    this->setWindowTitle(u"RAP: %1"_s.arg(artistAndTrack));
}

void MainWindow::saveSettings() {
    if (!settingsFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            tr("Can't open settings"),
            settingsFile.errorString()
        );
        return;
    }

    settings["lastDir"] = lastDir;
    settings["volume"] = volumeSlider->value();

    QJsonArray tabsArray;

    for (i8 i = 0; i < playlistView->tabCount() - 1; i++) {
        QJsonObject tabObject;

        const QString title = playlistView->tabText(i);
        tabObject.insert("title", title);

        const QString backgroundPath = playlistView->backgroundImagePath(i);
        tabObject.insert("backgroundPath", backgroundPath);

        auto* tree = playlistView->tree(i);
        const u32 rowCount = tree->model()->rowCount();

        QStringList trackPaths;
        trackPaths.reserve(rowCount);

        for (u32 row = 0; row < rowCount; row++) {
            trackPaths.append(getRowMetadata(tree, row)[Path]);
        }

        tabObject["paths"] = QJsonArray::fromStringList(trackPaths);
        tabsArray.append(tabObject);
    }

    settings["tabs"] = tabsArray;

    const auto [enabled, bandIndex, gains, frequencies] =
        equalizerMenu->getEqualizerInfo();

    QStringList gainsList;
    QStringList frequenciesList;

    for (const u8 gain : gains) {
        gainsList.append(QString::number(gain));
    }

    for (const f32 frequency : frequencies) {
        frequenciesList.append(QString::number(static_cast<f64>(frequency)));
    }

    QJsonArray equalizerArray;
    equalizerArray.append(enabled);
    equalizerArray.append(bandIndex);
    equalizerArray.append(QJsonArray::fromStringList(gainsList));
    equalizerArray.append(QJsonArray::fromStringList(frequenciesList));

    settings["equalizer"] = equalizerArray;

    settingsFile.write(QJsonDocument(settings).toJson());
    settingsFile.close();
}

void MainWindow::loadSettings() {
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

            const QString title = tabObject.value(u"title"_s).toString();
            const QString backgroundPath =
                tabObject.value(u"backgroundPath"_s).toString();

            const i8 index = playlistView->addTab(title);

            QJsonArray value = tabObject.value(u"paths"_s).toArray();
            QStringList pathStrings;
            pathStrings.reserve(value.size());

            for (const auto& path : value) {
                pathStrings.append(path.toString());
            }

            fillTable(playlistView->tree(index), pathStrings);

            if (!backgroundPath.isEmpty()) {
                playlistView->setBackgroundImage(index, backgroundPath);
            }
        }
    }

    const auto language = settings["language"];

    if (language.isString()) {
        // TODO: Change language
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
            const QAction* propertiesAction =
                menu->addAction(tr("Track Properties"));
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

            if (selectedAction != nullptr) {
                if (selectedAction == removeAction) {
                    trackTreeModel->removeRow(index.row());

                    trackTree->setCurrentIndex(
                        index.row() > currentIndex.row()
                            ? currentIndex
                            : trackTreeModel->index(currentIndex.row() - 1, 0)
                    );
                } else if (selectedAction == clearAction) {
                    trackTreeModel->removeRows(0, trackTreeModel->rowCount());
                } else if (selectedAction == propertiesAction) {
                    const auto* item = trackTreeModel->itemFromIndex(index);

                    if (item != nullptr) {
                        auto* metadataWindow = new MetadataWindow(
                            getRowMetadata(trackTree, index.row()),
                            this
                        );
                        metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                        metadataWindow->show();
                    }
                } else if (selectedAction == coverAction) {
                    const auto* item = trackTreeModel->itemFromIndex(index);

                    if (item != nullptr) {
                        const auto properties =
                            getRowMetadata(trackTree, index.row());

                        auto* coverWindow = new CoverWindow(
                            properties[Path],
                            properties[Title],
                            this
                        );

                        coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                        coverWindow->show();
                    }
                } else if (selectedAction == removeSelectionAction) {
                    selectedRows.removeOne(trackTree->currentIndex());

                    const QModelIndex& startRow = selectedRows.first();
                    const isize count = selectedRows.size();

                    trackTreeModel->removeRows(
                        startRow.row(),
                        static_cast<i32>(count)
                    );

                    trackTree->setCurrentIndex(
                        index.row() > currentIndex.row()
                            ? currentIndex
                            : trackTreeModel->index(
                                  static_cast<i32>(
                                      currentIndex.row() - selectedRows.size()
                                  ),
                                  0
                              )
                    );
                } else if (selectedAction == setPlaylistBackground) {
                    const QString file =
                        QFileDialog::getOpenFileName(this, tr("Select File"));

                    if (file.isEmpty()) {
                        return;
                    }

                    playlistView->setBackgroundImage(
                        playlistView->currentIndex(),
                        file
                    );
                } else if (selectedAction == removePlaylistBackground) {
                    playlistView->removeBackgroundImage(
                        playlistView->currentIndex()
                    );
                }
            }

            delete menu;
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
    if (button == Qt::LeftButton) {
    } else if (button == Qt::RightButton) {
        auto* menu = new OptionMenu(this);

        for (const QString& label : trackPropertiesLabels()) {
            auto* action = new QAction(label, menu);
            action->setCheckable(true);

            const u8 columnCount = trackTreeModel->columnCount();
            i8 columnIndex = -1;

            for (u8 column = 0; column < columnCount; column++) {
                if (trackTreeModel
                        ->headerData(column, Qt::Orientation::Horizontal)
                        .toString() == label) {
                    columnIndex = static_cast<i8>(column);
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

            // TODO: Make shitter
            if (prefix == "artist") {
                property = Artist;
            } else if (prefix == "no") {
                property = TrackNumber;
            } else if (prefix == "title") {
                property = Title;
            }
        }

        for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
            const QMap<u8, QString> metadata = getRowMetadata(trackTree, row);
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
    if (!progressSlider->isSliderDown()) {
        progressLabel->setText(u"%1/%2"_s.arg(toMinutes(second), audioDuration)
        );
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
    audioDuration = toMinutes(seconds);
    progressSlider->setRange(0, seconds);
    progressLabel->setText(u"0:00/%1"_s.arg(audioDuration));
}

void MainWindow::showSettingsWindow() {
    auto* settingsWindow = new SettingsWindow(this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->show();
}

void MainWindow::showHelpWindow() {
    auto* helpWindow = new HelpWindow(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->show();
}

void MainWindow::addFolder(const bool createNewTab) {
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Select Directory"),
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
            tr("Crazy path!"),
            tr("Cannot search by root or home path, to save your PC. Move music to a specific directory or navigate to an existing directory with music."
            )
        );
        return;
    }

    lastDir = directory;

    QDirIterator entries(
        directory,
        QDir::NoDotAndDotDot | QDir::Files,
        QDirIterator::Subdirectories
    );

    auto* tree = trackTree;

    if (createNewTab) {
        const i8 index = playlistView->addTab(QFileInfo(directory).fileName());
        tree = playlistView->tree(index);
    }

    fillTable(tree, entries);
}

void MainWindow::addFile(const bool createNewTab) {
    const QString file = QFileDialog::getOpenFileName(this, tr("Select File"));
    if (file.isEmpty()) {
        return;
    }

    auto* tree = trackTree;
    if (createNewTab) {
        const i8 index = playlistView->addTab(QFileInfo(file).fileName());
        tree = playlistView->tree(index);
    }

    fillTable(trackTree, { file });
}

void MainWindow::togglePlayback() {
    const QAudio::State state = audioWorker->state();

    if (state != QAudio::IdleState) {
        if (state == QAudio::ActiveState) {
            actionPause->trigger();
        } else {
            actionResume->trigger();
        }
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
    const QString progress =
        u"%1/%2"_s.arg(toMinutes(progressSlider->value()), audioDuration);
    progressLabel->setText(progress);
}

void MainWindow::updateVolume(const u16 value) {
    volumeLabel->setText(u"%1%"_s.arg(value));
    audioWorker->setVolume(static_cast<f32>(value) / 100.0F);
}

void MainWindow::cancelSearchInput() {
    searchTrackInput->clear();
    searchTrackInput->hide();
}

void MainWindow::initializeEqualizerMenu() {
    equalizerMenu = new EqualizerMenu(eqButton, audioWorker);

    if (settings["equalizer"].isArray()) {
        const auto gainsList = settings["equalizer"][2].toArray();
        vector<i8> gains(gainsList.size());

        for (const auto [idx, gain] : views::enumerate(gainsList)) {
            gains[idx] = static_cast<i8>(gain.toInt());
        }

        const auto frequenciesList = settings["equalizer"][3].toArray();
        vector<f32> frequencies(frequenciesList.size());

        for (const auto [idx, frequency] : views::enumerate(frequenciesList)) {
            frequencies[idx] = static_cast<f32>(frequency.toDouble());
        }

        equalizerMenu->setEqualizerInfo(
            settings["equalizer"][0].toBool(),
            settings["equalizer"][1].toInt(),
            gains,
            frequencies
        );
    }
}

void MainWindow::toggleEqualizerMenu(const bool checked) {
    if (checked) {
        equalizerMenu->move(eqButton->mapToGlobal(QPoint(0, eqButton->height()))
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
    saveSettings();
    QApplication::quit();
}

void MainWindow::onTrayIconActivated(
    const QSystemTrayIcon::ActivationReason reason
) {
    if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
        this->show();
        this->raise();
        this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
    }
}

void MainWindow::setupTrayIcon() {
    trayIcon->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh));
    trayIcon->show();

    trayIconMenu->addActions(
        { actionResume,
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
            trackTreeModel->item(oldRow, 0)->setText("");
        }

        playingPlaylist = playlistView->currentIndex();
        playTrack(trackTree, trackTreeModel->index(newRow, 0));
    }

    trackTree->clearSelection();
}

void MainWindow::resetSorting(const i32 index, const Qt::SortOrder sortOrder) {
    const i32 section = trackTreeHeader->sortIndicatorSection();

    if (section == -1) {
        for (i8 column = 0; column < trackTreeModel->columnCount(); column++) {
            if (trackTreeModel->trackProperty(column) == Path) {
                trackTree->sortByColumn(column, Qt::SortOrder::AscendingOrder);
                break;
            }
        }
    }
};