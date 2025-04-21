#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audioworker.hpp"
#include "constants.hpp"
#include "custominput.hpp"
#include "customslider.hpp"
#include "equalizermenu.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <random>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Drag tracks in tree to arrange them as user wants

constexpr u16 TRACK_PROPERTIES_ARRAY_SIZE = UINT8_MAX + 1;

constexpr auto
propertyHash(const u8 charA, const u8 charB, const u8 charC, const u8 charD)
    -> u8 {
    return static_cast<u8>(charA + charB + charC + charD);
}

constexpr auto initTrackProperties()
    -> array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> {
    array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> arr = {};

    arr[propertyHash('T', 'i', 'l', 'e')] = Title;
    arr[propertyHash('A', 'r', 's', 't')] = Artist;
    arr[propertyHash('A', 'l', 'u', 'm')] = Album;
    arr[propertyHash('T', 'r', 'e', 'r')] = TrackNumber;
    arr[propertyHash('A', 'l', 's', 't')] = AlbumArtist;
    arr[propertyHash('G', 'e', 'r', 'e')] = Genre;
    arr[propertyHash('Y', 'e', 'a', 'r')] = Year;
    arr[propertyHash('D', 'u', 'o', 'n')] = Duration;
    arr[propertyHash('C', 'o', 'e', 'r')] = Composer;
    arr[propertyHash('B', 'P', 'P', 'M')] = BPM;
    arr[propertyHash('L', 'a', 'g', 'e')] = Language;
    arr[propertyHash('D', 'i', 'e', 'r')] = DiscNumber;
    arr[propertyHash('C', 'o', 'n', 't')] = Comment;
    arr[propertyHash('P', 'u', 'e', 'r')] = Publisher;
    arr[propertyHash('B', 'i', 't', 'e')] = Bitrate;
    arr[propertyHash('S', 'a', 't', 'e')] = SampleRate;
    arr[propertyHash('C', 'h', 'l', 's')] = Channels;
    arr[propertyHash('F', 'o', 'a', 't')] = Format;

    return arr;
}

constexpr array<TrackProperty, TRACK_PROPERTIES_ARRAY_SIZE> TRACK_PROPERTIES =
    initTrackProperties();

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void filesDropped(const QStringList& filePaths);
    void trackDataReady(
        TrackTree* tree,
        const metadata_array& metadata,
        const QString& path
    );
    void trackDataProcessed(TrackTree* tree);

   private:
    void saveSettings();
    void parseSettings();
    auto setupUi() -> bool;

    // Inline utility functions
    inline void fillTable(TrackTree* tree, const QString& filePath);
    inline void fillTable(TrackTree* tree, const QStringList& paths);
    inline void fillTable(TrackTree* tree, QDirIterator& iterator);
    inline void jumpToTrack(Direction direction, bool clicked);
    inline void updatePlaybackPosition();
    inline void playTrack(const QModelIndex& index);
    inline void stopPlayback();
    static inline auto getTrackProperty(const QString& propertyString)
        -> TrackProperty;

    // UI - Widgets
    Ui::MainWindow* ui = new Ui::MainWindow();
    // GENIUS
    const bool g = setupUi();
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    QMenu* trayIconMenu = new QMenu(this);
    TrackTree* trackTree = nullptr;
    MusicHeader* trackTreeHeader = nullptr;
    MusicModel* trackTreeModel = nullptr;
    QLabel* progressLabel = ui->progressLabel;
    QLabel* volumeLabel = ui->volumeLabel;
    CustomInput* searchTrackInput = new CustomInput(this);
    QLabel* trackLabel = ui->trackLabel;
    PlaylistView* playlistView = ui->playlistView;
    PlaylistTabBar* playlistTabBar =
        static_cast<PlaylistTabBar*>(ui->playlistView->tabBar());

    // UI - Buttons
    QPushButton* playButton = ui->playButton;
    QPushButton* stopButton = ui->stopButton;
    QPushButton* backwardButton = ui->backwardButton;
    QPushButton* forwardButton = ui->forwardButton;
    ActionButton* repeatButton = ui->repeatButton;
    ActionButton* randomButton = ui->randomButton;
    QPushButton* eqButton = ui->eqButton;

    // UI - Sliders
    CustomSlider* volumeSlider = ui->volumeSlider;
    CustomSlider* progressSlider = ui->progressSlider;

    // UI - Icons
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");

    // Actions & Menus
    QMenu* fileMenu = ui->menuFile;
    QAction* exitAction = ui->actionExit;
    QAction* openFileAction = ui->actionOpenFile;
    QAction* openFolderAction = ui->actionOpenFolder;
    QAction* aboutAction = ui->actionAbout;
    QAction* forwardAction = ui->actionForward;
    QAction* backwardAction = ui->actionBackward;
    QAction* repeatAction = ui->actionRepeat;
    QAction* pauseAction = ui->actionPause;
    QAction* resumeAction = ui->actionResume;
    QAction* stopAction = ui->actionStop;
    QAction* randomAction = ui->actionRandom;
    QAction* settingsAction = ui->actionSettings;
    QAction* helpAction = ui->actionHelp;
    QAction* addFileAction = ui->actionAddFile;
    QAction* addFolderAction = ui->actionAddFolder;

    // Settings
    QJsonObject settings;
    QString lastDir;

    // UI State
    QStringList headerLabels = { "Title", "Artist", "Track Number" };

    // Audio
    QString audioDuration = "0:00";

    // Control Logic
    RepeatMode repeat = RepeatMode::Off;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;
    IndexSet playHistory;

    // Threads
    QThreadPool* threadPool = new QThreadPool();
    AudioWorker* audioWorker = new AudioWorker();

    // Search
    vector<QModelIndex> searchMatches;
    usize searchMatchesPosition = 0;
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    QString previousSearchPrompt;

    // Miscellaneous
    std::random_device randdevice;
    QFile settingsFile =
        QApplication::applicationDirPath() + "/" + "rap-settings.json";
    EqualizerMenu* equalizerMenu = nullptr;
};
