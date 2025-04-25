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
#include "metadatawindow.hpp"
#include "musicheader.hpp"
#include "musicitem.hpp"
#include "randint.hpp"
#include "settingswindow.hpp"
#include "tominutes.hpp"
#include "trackpropertiesmap.hpp"
#include "tracktree.hpp"

#include <QDirIterator>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMimeData>
#include <QShortcut>
#include <qitemselectionmodel.h>
#include <qnamespace.h>
#include <ranges>

constexpr u8 MINIMUM_MATCHES_NUMBER = 32;
constexpr u8 SEARCH_INPUT_MIN_WIDTH = 64;
constexpr u8 SEARCH_INPUT_HEIGHT = 24;

auto MainWindow::setupUi() -> Ui::MainWindow* {
    auto* _ui = new Ui::MainWindow();
    _ui->setupUi(this);
    return _ui;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    connect(
        eqButton,
        &QPushButton::toggled,
        this,
        &MainWindow::toggleEqualizerMenu
    );

    connect(forwardAction, &QAction::triggered, this, &MainWindow::jumpForward);

    connect(
        backwardAction,
        &QAction::triggered,
        this,
        &MainWindow::jumpBackward
    );

    connect(repeatAction, &QAction::triggered, this, &MainWindow::toggleRepeat);

    connect(pauseAction, &QAction::triggered, this, &MainWindow::pausePlayback);

    connect(
        resumeAction,
        &QAction::triggered,
        this,
        &MainWindow::resumePlayback
    );

    connect(
        stopAction,
        &QAction::triggered,
        this,
        &MainWindow::stopPlaybackAndClear
    );

    connect(randomAction, &QAction::triggered, this, &MainWindow::toggleRandom);

    connect(exitAction, &QAction::triggered, this, &MainWindow::exit);

    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &MainWindow::onTrayIconActivated
    );

    connect(openFolderAction, &QAction::triggered, this, [&] {
        addFolder(true);
    });

    connect(openFileAction, &QAction::triggered, this, [&] { addFile(true); });

    connect(addFolderAction, &QAction::triggered, this, [&] {
        addFolder(false);
    });

    connect(addFileAction, &QAction::triggered, this, [&] { addFile(false); });

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
        aboutAction,
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

    connect(
        audioWorker,
        &AudioWorker::endOfFile,
        this,
        &MainWindow::onEndOfFile
    );

    connect(
        audioWorker,
        &AudioWorker::duration,
        this,
        &MainWindow::updateProgress
    );

    connect(
        settingsAction,
        &QAction::triggered,
        this,
        &MainWindow::showSettings
    );

    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);

    connect(
        playlistView,
        &PlaylistView::indexChanged,
        this,
        &MainWindow::changePlaylist
    );

    setAcceptDrops(true);

    searchMatches.reserve(MINIMUM_MATCHES_NUMBER);

    setupTrayIcon();

    repeatButton->setAction(repeatAction);
    randomButton->setAction(randomAction);

    searchTrackInput->hide();
    searchTrackInput->setPlaceholderText("Track Name");
    searchTrackInput->setMinimumWidth(MINIMUM_MATCHES_NUMBER);
    searchTrackInput->setFixedHeight(SEARCH_INPUT_HEIGHT);

    initializeEqualizerMenu();

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
        for (const QUrl& url : event->mimeData()->urls()) {
            if (!url.isEmpty()) {
                processFile(trackTree, url.toLocalFile());
            }
        }
    }
}

void MainWindow::processFile(TrackTree* tree, const QString& filePath) {
    for (const auto& extension : ALLOWED_EXTENSIONS) {
        if (!filePath.endsWith(extension)) {
            return;
        }

        auto* model = tree->model();

        auto metadata = extractMetadata(filePath.toStdString().c_str());

        const u16 row = model->rowCount();
        model->setRowMetadata(row, metadata);

        for (u16 column = 0; column < model->columnCount(); column++) {
            auto* item = new MusicItem();

            if (column == 0) {
                item->setPath(filePath);
            }

            const QString& propertyString =
                model->headerData(column, Qt::Orientation::Horizontal)
                    .toString();
            const TrackProperty property = getTrackProperty(propertyString);

            if (property == TrackProperty::TrackNumber) {
                QString number;

                for (const auto& [idx, chr] :
                     views::enumerate(metadata[property])) {
                    if (idx == 0 && chr == '0') {
                        continue;
                    }

                    if (!chr.isDigit()) {
                        break;
                    }

                    number.append(chr);
                }

                item->setData(number.toInt(), Qt::EditRole);
            } else {
                item->setText(metadata[property]);
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
                    stopPlayback();
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
    trackLabel->clear();
    setWindowTitle(u"RAP"_s);
    audioWorker->stop();
    audioDuration = "0:00";
    progressSlider->setRange(0, 0);
    progressLabel->setText(u"0:00/0:00"_s);
    playButton->setIcon(startIcon);
    trackTree->clearSelection();
}

void MainWindow::playTrack(const QModelIndex& index) {
    playButton->setIcon(pauseIcon);

    const u16 row = index.row();

    QMetaObject::invokeMethod(
        audioWorker,
        &AudioWorker::start,
        Qt::QueuedConnection,
        trackTreeModel->rowProperty(row, TrackProperty::Path)
    );

    const QString artistAndTrack = u"%1 - %2"_s.arg(
        trackTreeModel->rowProperty(row, TrackProperty::Artist),
        trackTreeModel->rowProperty(row, TrackProperty::Title)
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
    if (!settingsFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this,
            u"Can't open settings"_s,
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

        auto* tree = playlistView->tree(i);

        // TODO: Fix panic
        const u32 rowCount = tree->model()->rowCount();

        QStringList paths;
        paths.reserve(rowCount);

        for (u32 row = 0; row < rowCount; row++) {
            paths.append(tree->model()->rowProperty(row, TrackProperty::Path));
        }

        tabObject["paths"] = QJsonArray::fromStringList(paths);
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

    QJsonArray array;
    array.append(enabled);
    array.append(bandIndex);
    array.append(QJsonArray::fromStringList(gainsList));
    array.append(QJsonArray::fromStringList(frequenciesList));

    settings["equalizer"] = array;

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

            const QString title = tabObject.value(u"title"_s).toString();
            const i8 index = playlistView->addTab(title);

            QJsonArray value = tabObject.value(u"paths"_s).toArray();
            QStringList pathStrings;
            pathStrings.reserve(value.size());

            for (const auto& path : value) {
                pathStrings.append(path.toString());
            }

            fillTable(playlistView->tree(index), pathStrings);
        }
    }
}

void MainWindow::handleTrackPress(const QModelIndex& index) {
    switch (QApplication::mouseButtons()) {
        case Qt::RightButton: {
            auto* menu = new QMenu(this);

            const QAction* removeAction = menu->addAction("Remove Track");

            const QModelIndexList selectedRows =
                trackTree->selectionModel()->selectedRows();
            const QAction* removeSelectionAction = nullptr;

            if (selectedRows.size() > 1) {
                removeSelectionAction = menu->addAction("Remove Selection");
            }

            const QAction* clearAction = menu->addAction("Clear All Tracks");
            const QAction* propertiesAction =
                menu->addAction("Track Properties");
            const QAction* coverAction = menu->addAction("Show Cover");

            const QAction* selectedAction = menu->exec(QCursor::pos());
            menu->deleteLater();

            if (selectedAction == removeAction) {
                if (index.isValid()) {
                    trackTreeModel->removeRowMetadata(index.row());
                    trackTreeModel->removeRow(index.row());
                }
            } else if (selectedAction == clearAction) {
                trackTreeModel->removeRows(0, trackTreeModel->rowCount());
                trackTreeModel->clearRowMetadata();
            } else if (selectedAction == propertiesAction) {
                const auto* item = trackTreeModel->itemFromIndex(index);

                if (item != nullptr) {
                    auto* metadataWindow = new MetadataWindow(
                        trackTreeModel->rowMetadata(index.row()),
                        this
                    );
                    metadataWindow->setAttribute(Qt::WA_DeleteOnClose);
                    metadataWindow->show();
                }
            } else if (selectedAction == coverAction) {
                const auto* item = trackTreeModel->itemFromIndex(index);

                if (item != nullptr) {
                    auto* coverWindow = new CoverWindow(
                        trackTreeModel
                            ->rowProperty(index.row(), TrackProperty::Path),
                        trackTreeModel
                            ->rowProperty(index.row(), TrackProperty::Title),
                        this
                    );

                    coverWindow->setAttribute(Qt::WA_DeleteOnClose);
                    coverWindow->show();
                }
            } else if (selectedAction == removeSelectionAction) {
                for (const auto& selectedRow :
                     ranges::reverse_view(selectedRows)) {
                    if (trackTree->currentIndex() == selectedRow) {
                        continue;
                    }

                    trackTreeModel->removeRow(selectedRow.row());
                    trackTreeModel->removeRowMetadata(selectedRow.row());
                }

                // TODO: Set correct index
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

auto MainWindow::createHeaderContextMenu() -> OptionMenu* {
    auto* menu = new OptionMenu(this);
    auto& headerLabels = playlistView->headerLabels();

    for (const auto& [label, property] : TRACK_PROPERTIES_MAP) {
        auto* action = new QAction(label, menu);
        action->setCheckable(true);

        if (headerLabels.contains(label)) {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, menu, [=, this] {
            auto& headerLabels = playlistView->headerLabels();
            const i8 index = static_cast<i8>(headerLabels.indexOf(label));

            if (index != -1) {
                headerLabels.removeAt(index);

                for (u8 column = 0; column < trackTreeModel->columnCount();
                     column++) {
                    const QString columnLabel =
                        trackTreeModel
                            ->headerData(column, Qt::Orientation::Horizontal)
                            .toString();

                    if (label == columnLabel) {
                        trackTreeModel->removeColumn(column);
                        break;
                    }
                }
            } else {
                headerLabels.append(label);
                const u8 column = headerLabels.size();

                for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
                    auto* item =
                        new MusicItem(trackTreeModel->rowProperty(row, property)
                        );
                    trackTreeModel->setItem(row, column - 1, item);
                }

                for (u8 column = 0; column < trackTreeModel->columnCount();
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

    return menu;
}

void MainWindow::handleHeaderPress(
    const u8 index,
    const Qt::MouseButton button
) {
    if (button == Qt::LeftButton) {
    } else if (button == Qt::RightButton) {
        auto* menu = createHeaderContextMenu();
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
        TrackProperty property = TrackProperty::Title;

        if (colonPos != -1) {
            prefix = input.sliced(0, colonPos);
            value = input.sliced(colonPos + 1);
            hasPrefix = true;

            if (prefix == "no") {
                prefix = "Trer";  // for computing a hash
            }

            property = getTrackProperty(prefix);
        }

        for (u16 row = 0; row < trackTreeModel->rowCount(); row++) {
            const metadata_array& metadata = trackTreeModel->rowMetadata(row);
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

void MainWindow::onEndOfFile() {
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

void MainWindow::showSettings() {
    auto* settingsWindow = new SettingsWindow(this);
    settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
    settingsWindow->show();
}

void MainWindow::showHelp() {
    auto* helpWindow = new SettingsWindow(this);
    helpWindow->setAttribute(Qt::WA_DeleteOnClose);
    helpWindow->show();
}

void MainWindow::addFolder(const bool createNewTab) {
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
            "Cannot search by root or home path, to save your PC. Move music to a specific directory or navigate to an existing directory with music."
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
    const QString file = QFileDialog::getOpenFileName(this, "Select File");
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
    if (audioWorker->playing()) {
        pauseAction->trigger();
    } else {
        resumeAction->trigger();
    }
}

void MainWindow::moveForward() {
    forwardAction->trigger();
}

void MainWindow::moveBackward() {
    backwardAction->trigger();
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
        db_gains_array gains;

        for (const auto [idx, gain] : views::enumerate(gainsList)) {
            gains[idx] = static_cast<i8>(gain.toInt());
        }

        const auto frequenciesList = settings["equalizer"][3].toArray();
        frequencies_array frequencies;

        for (const auto [idx, frequency] : views::enumerate(frequenciesList)) {
            frequencies[idx] = static_cast<f32>(frequency.toDouble());
        }

        equalizerMenu->setEqInfo(
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

void MainWindow::stopPlaybackAndClear() {
    stopPlayback();
    trackTree->clearSelection();
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
}

void MainWindow::changePlaylist(const i8 index) {
    qDebug() << "change playlist called";
    if (index == -1) {
        trackTree = nullptr;
        trackTreeHeader = nullptr;
        trackTreeModel = nullptr;
        return;
    }

    trackTree = playlistView->tree(index);
    trackTreeHeader = trackTree->header();
    trackTreeModel = trackTree->model();

    trackTree->setSortingEnabled(true);
    trackTreeHeader->setSectionsClickable(true);
    trackTreeHeader->setSectionsMovable(true);
    trackTreeHeader->setSortIndicatorShown(true);
    trackTreeHeader->setSortIndicatorClearable(true);

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

    connect(trackTreeHeader, &MusicHeader::sortIndicatorChanged, this, [&] {
        sortIndicatorCleared++;

        if (sortIndicatorCleared == 3) {
            // TODO: Sort by path
            sortIndicatorCleared = 0;
        }
    });

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
        const auto requiredIndex = trackTree->currentIndex();

        if (deselected.contains(requiredIndex)) {
            trackTree->selectionModel()->select(
                requiredIndex,
                QItemSelectionModel::Select | QItemSelectionModel::Rows
            );
        }
    }
    );
}

void MainWindow::selectTrack(
    const QModelIndex& oldIndex,
    const QModelIndex& newIndex
) {
    if (newIndex != oldIndex) {
        playTrack(newIndex);
    }
}