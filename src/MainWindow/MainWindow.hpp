#pragma once

#include "ActionButton.hpp"
#include "Aliases.hpp"
#include "AudioWorker.hpp"
#include "Constants.hpp"
#include "CustomInput.hpp"
#include "CustomSlider.hpp"
#include "DockWidget.hpp"
#include "EqualizerMenu.hpp"
#include "IconTextButton.hpp"
#include "IndexSet.hpp"
#include "MusicHeader.hpp"
#include "OptionMenu.hpp"
#include "PeakVisualizer.hpp"
#include "PlaylistView.hpp"
#include "RepeatRangeMenu.hpp"
#include "ScaledLabel.hpp"
#include "Settings.hpp"
#include "TrackTree.hpp"
#include "TrackTreeModel.hpp"
#include "ui_MainWindow.h"

#ifdef PROJECTM
#include "VisualizerDialog.hpp"
#endif

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

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(const QStringList& paths, QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   private:
    inline auto saveSettings() -> result<bool, QString>;
    inline auto savePlaylists() -> result<bool, QString>;
    inline void loadPlaylists();
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
    inline void togglePlayback(
        const QString& path = QString(),
        const u16 startSecond = UINT16_MAX
    );
    inline void showAboutWindow();
    inline void processDroppedFiles(const QStringList& files);
    inline void
    updateProgressLabel(u16 second, const QString& duration = QString());
    inline void updateVolume(u8 value);
    inline void cancelSearchInput();
    inline void toggleEqualizerMenu();
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
    inline void importPlaylist(bool createNewTab, QString filePath = QString());
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
        auto* const ui_ = new Ui::MainWindow();
        ui_->setupUi(this);
        return ui_;
    };

    // UI - Widgets
    Ui::MainWindow* const ui = setupUi();

    // Tray Icon
    QSystemTrayIcon* const trayIcon = new QSystemTrayIcon(this);
    OptionMenu* const trayIconMenu = new OptionMenu(this);
    QLabel* const progressLabelTray = new QLabel(trayIconMenu);
    QLabel* const volumeLabelTray = new QLabel(trayIconMenu);
    CustomSlider* const volumeSliderTray =
        new CustomSlider(Qt::Horizontal, trayIconMenu);
    CustomSlider* const progressSliderTray =
        new CustomSlider(Qt::Horizontal, trayIconMenu);

    // Labels
    QLabel* const progressLabel = ui->progressLabel;
    QLabel* const volumeLabel = ui->volumeLabel;
    QLabel* const trackLabel = ui->trackLabel;

    // Playlist
    PlaylistView* const playlistView = ui->playlistView;
    PlaylistTabBar* const playlistTabBar = playlistView->tabBar();
    TrackTree* trackTree = nullptr;
    MusicHeader* trackTreeHeader = nullptr;
    TrackTreeModel* trackTreeModel = nullptr;

    // UI - Buttons
    ActionButton* const playButton = ui->playButton;
    ActionButton* const stopButton = ui->stopButton;
    ActionButton* const backwardButton = ui->backwardButton;
    ActionButton* const forwardButton = ui->forwardButton;
    IconTextButton* const repeatButton = ui->repeatButton;
    ActionButton* const randomButton = ui->randomButton;
    QPushButton* const equalizerButton = ui->equalizerButton;

    // UI - Sliders
    CustomSlider* const volumeSlider = ui->volumeSlider;
    CustomSlider* const progressSlider = ui->progressSlider;

    // UI - Icons
    const QIcon pauseIcon = QIcon::fromTheme(u"media-playback-pause"_s);
    const QIcon startIcon = QIcon::fromTheme(u"media-playback-start"_s);

    // Actions & Menus
    const QAction* const actionOpenFile = ui->actionOpenFile;
    const QAction* const actionOpenFolder = ui->actionOpenFolder;
    const QAction* const actionOpenPlaylist = ui->actionOpenPlaylist;
    const QAction* const actionSettings = ui->actionSettings;
    const QAction* const actionVisualizer = ui->actionVisualizer;
    QAction* actionExit = ui->actionExit;

    const QAction* const actionAddFile = ui->actionAddFile;
    const QAction* const actionAddFolder = ui->actionAddFolder;
    const QAction* const actionAddPlaylist = ui->actionAddPlaylist;

    const QAction* const actionExportXSPFPlaylist =
        ui->actionExportXSPFPlaylist;
    const QAction* const actionExportM3U8Playlist =
        ui->actionExportM3U8Playlist;

    const QAction* const actionRussian = ui->actionRussian;
    const QAction* const actionEnglish = ui->actionEnglish;

    const QAction* const actionAbout = ui->actionAbout;
    const QAction* const actionDocumentation = ui->actionDocumentation;

    QAction* actionForward = ui->actionForward;
    QAction* actionBackward = ui->actionBackward;
    QAction* actionRepeat = ui->actionRepeat;
    QAction* actionTogglePlayback = ui->actionTogglePlayback;
    QAction* actionStop = ui->actionStop;
    QAction* actionRandom = ui->actionRandom;

    // Settings
    shared_ptr<Settings> settings;

    // UI State
    const QString ZERO_DURATION = u"00:00"_s;
    const QString FULL_ZERO_DURATION = u"00:00/00:00"_s;
    QString trackDuration = FULL_ZERO_DURATION;
    QTranslator* translator = new QTranslator(this);

    // Control Logic
    bool random = false;
    RepeatMode repeat = RepeatMode::Off;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;
    i8 playingPlaylist = -1;
    IndexSet playHistory;

    QSplitter* const mainArea = ui->mainArea;
    DockWidget* const dockWidget = ui->dockWidget;
    ScaledLabel* const dockCoverLabel = ui->dockCoverLabel;
    QTreeWidget* const dockMetadataTree = ui->dockMetadataTree;

    // Threads
    QThreadPool* const threadPool = new QThreadPool();

    // Audio
    array<f32, MIN_BUFFER_SIZE / F32_SAMPLE_SIZE> visualizerBuffer;
    array<f32, MIN_BUFFER_SIZE / F32_SAMPLE_SIZE> peakVisualizerBuffer;

    PeakVisualizer* const peakVisualizer =
        new PeakVisualizer(peakVisualizerBuffer.data(), this);

    AudioWorker* audioWorker;

    // Search
    CustomInput* const searchTrackInput = new CustomInput(this);
    QVector<QModelIndex> searchMatches;
    isize searchMatchesPosition = 0;
    const QShortcut* const searchShortcut =
        new QShortcut(QKeySequence(u"Ctrl+F"_s), this);
    QString previousSearchPrompt;

    // Miscellaneous
    QString currentTrack;
    u16 CUEOffset = UINT16_MAX;

    std::random_device seed;
    std::mt19937 rng = std::mt19937(seed());

    QLocalServer* const server = new QLocalServer(this);

    RepeatRangeMenu* const repeatRangeMenu = new RepeatRangeMenu(this);
    EqualizerMenu* equalizerMenu = nullptr;

#ifdef PROJECTM
    VisualizerDialog* visualizerDialog = nullptr;
#endif
};
