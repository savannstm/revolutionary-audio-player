#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audioworker.hpp"
#include "custominput.hpp"
#include "customslider.hpp"
#include "dockwidget.hpp"
#include "equalizermenu.hpp"
#include "icontextbutton.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"
#include "optionmenu.hpp"
#include "peakvisualizer.hpp"
#include "playlistview.hpp"
#include "scaledlabel.hpp"
#include "settings.hpp"
#include "tracktree.hpp"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QLabel>
#include <QLocalServer>
#include <QMainWindow>
#include <QPushButton>
#include <QShortcut>
#include <QString>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTranslator>
#include <random>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Support .cue
// TODO: Add "play slice" thing

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(const QStringList& paths, QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void retranslated();

   private:
    inline auto saveSettings() -> result<bool, QString>;
    inline void initializeSettings();
    inline void loadSettings();
    inline void advancePlaylist(Direction direction);
    inline void updatePlaybackPosition();
    inline void playTrack(TrackTree* tree, const QModelIndex& index);
    inline void stopPlayback();
    inline void handleTrackPress(const QModelIndex& index);
    inline void searchTrack();
    inline void showSearchInput();
    inline void onAudioProgressUpdated(u16 second);
    inline void playNext();
    inline void updateProgress(u16 seconds);
    inline void showSettingsWindow();
    inline void showHelpWindow();
    inline void addEntry(bool createNewTab, bool isFolder);
    inline void togglePlayback(const QString& path = QString());
    inline void showAboutWindow();
    inline void processDroppedFiles(const QStringList& files);
    inline void updateProgressLabel();
    inline void updateVolume(u16 value);
    inline void cancelSearchInput();
    inline void toggleEqualizerMenu(bool checked);
    inline void toggleRepeat();
    inline void toggleRandom();
    inline void exit();
    inline void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    inline void setupTrayIcon();
    inline auto changePlaylist(i8 index) -> bool;
    inline void closeTab(i8 index);
    inline void selectTrack(i32 oldRow, u16 newRow);
    inline void resetSorting(i32 index, Qt::SortOrder sortOrder);
    inline void retranslate(QLocale::Language language = QLocale::AnyLanguage);
    inline void moveDockWidget(DockWidgetPosition dockWidgetPosition);
    inline void processArgs(const QStringList& args);
    inline void focus();
    inline void importPlaylist(bool createNewTab);
    inline void exportPlaylist();
    inline void adjustPlaylistView();
    inline void adjustPlaylistTabBar(
        DockWidgetPosition dockWidgetPosition,
        u8 currentIndex
    );
    inline void
    adjustPlaylistImage(DockWidgetPosition dockWidgetPosition, u8 currentIndex);

    inline void exportPlaylist(PlaylistFileType playlistType);
    static inline auto exportXSPF(
        const QString& outputPath,
        const vector<HashMap<TrackProperty, QString>>& metadataVector
    ) -> result<bool, QString>;
    static inline auto exportM3U8(
        const QString& outputPath,
        const vector<HashMap<TrackProperty, QString>>& metadataVector
    ) -> result<bool, QString>;

    static inline auto constructAudioFileFilter() -> QString;

    auto setupUi() -> Ui::MainWindow* {
        auto* ui_ = new Ui::MainWindow();
        ui_->setupUi(this);
        return ui_;
    };

    // UI - Widgets
    Ui::MainWindow* ui = setupUi();

    // Tray Icon
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    OptionMenu* trayIconMenu = new OptionMenu(this);
    QLabel* progressLabelCloned = new QLabel(trayIconMenu);
    QLabel* volumeLabelCloned = new QLabel(trayIconMenu);
    CustomSlider* volumeSliderCloned =
        new CustomSlider(Qt::Horizontal, trayIconMenu);
    CustomSlider* progressSliderCloned =
        new CustomSlider(Qt::Horizontal, trayIconMenu);

    // Labels
    QLabel* progressLabel = ui->progressLabel;
    QLabel* volumeLabel = ui->volumeLabel;
    QLabel* trackLabel = ui->trackLabel;

    // Playlist
    PlaylistView* playlistView = ui->playlistView;
    PlaylistTabBar* playlistTabBar = playlistView->tabBar();
    TrackTree* trackTree = nullptr;
    MusicHeader* trackTreeHeader = nullptr;
    MusicModel* trackTreeModel = nullptr;

    // UI - Buttons
    ActionButton* playButton = ui->playButton;
    ActionButton* stopButton = ui->stopButton;
    ActionButton* backwardButton = ui->backwardButton;
    ActionButton* forwardButton = ui->forwardButton;
    IconTextButton* repeatButton = ui->repeatButton;
    ActionButton* randomButton = ui->randomButton;
    QPushButton* equalizerButton = ui->equalizerButton;

    // UI - Sliders
    CustomSlider* volumeSlider = ui->volumeSlider;
    CustomSlider* progressSlider = ui->progressSlider;

    // UI - Icons
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");

    // Actions & Menus
    QAction* actionOpenFile = ui->actionOpenFile;
    QAction* actionOpenFolder = ui->actionOpenFolder;
    QAction* actionOpenPlaylist = ui->actionOpenPlaylist;
    QAction* actionExit = ui->actionExit;

    QAction* actionAddFile = ui->actionAddFile;
    QAction* actionAddFolder = ui->actionAddFolder;
    QAction* actionAddPlaylist = ui->actionAddPlaylist;

    QAction* actionExportXSPFPlaylist = ui->actionExportXSPFPlaylist;
    QAction* actionExportM3U8Playlist = ui->actionExportM3U8Playlist;

    QAction* actionSettings = ui->actionSettings;

    QAction* actionRussian = ui->actionRussian;
    QAction* actionEnglish = ui->actionEnglish;

    QAction* actionAbout = ui->actionAbout;
    QAction* actionDocumentation = ui->actionDocumentation;

    QAction* actionForward = ui->actionForward;
    QAction* actionBackward = ui->actionBackward;
    QAction* actionRepeat = ui->actionRepeat;
    QAction* actionTogglePlayback = ui->actionTogglePlayback;
    QAction* actionStop = ui->actionStop;
    QAction* actionRandom = ui->actionRandom;

    // Settings
    shared_ptr<Settings> settings;

    // UI State
    const QString ZERO_DURATION = u"0:00/0:00"_s;
    QString trackDuration = ZERO_DURATION;
    QTranslator* translator = new QTranslator(this);

    // Control Logic
    RepeatMode repeat = RepeatMode::Off;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;
    IndexSet playHistory;
    i8 playingPlaylist = -1;

    QSplitter* mainArea = ui->mainArea;
    DockWidget* dockWidget = ui->dockWidget;
    ScaledLabel* dockCoverLabel = ui->dockCoverLabel;
    QTreeWidget* dockMetadataTree = ui->dockMetadataTree;

    // Threads
    QThreadPool* threadPool = new QThreadPool();
    AudioWorker* audioWorker = new AudioWorker();

    // Search
    CustomInput* searchTrackInput = new CustomInput(this);
    QVector<QModelIndex> searchMatches;
    isize searchMatchesPosition = 0;
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    QString previousSearchPrompt;

    // Settings
    QFile settingsFile =
        QFile(QApplication::applicationDirPath() + "/rap-settings.json");

    // Miscellaneous
    EqualizerMenu* equalizerMenu = nullptr;
    u8 sortIndicatorCleared;
    std::random_device seed;
    std::mt19937 rng = std::mt19937(seed());
    QLocalServer* server = new QLocalServer(this);
    QAudioDevice previousDefaultAudioDevice =
        QMediaDevices::defaultAudioOutput();
    PeakVisualizer* peakVisualizer = new PeakVisualizer(this);
};
