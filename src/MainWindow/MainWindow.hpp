#pragma once

#include "Aliases.hpp"
#include "DockWidget.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "PlaylistView.hpp"

#include <QLocale>
#include <QMainWindow>
#include <QSystemTrayIcon>

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
    inline void advancePlaylist(PlaylistView::Direction direction);
    inline void updatePlaybackPosition();
    inline void playTrack(TrackTable* table, i32 row);
    inline void stopPlayback();
    inline void handleTrackPress(const QModelIndex& index);
    inline void searchTrack();
    inline void toggleSearchInput(bool forceShow = false);
    inline void onAudioProgressUpdated(i32 second);
    inline void playNext();
    inline void updateProgress(i32 seconds);
    inline void showSettingsWindow();
    inline void addEntry(bool createNewTab, bool isFolder);
    inline void
    togglePlayback(const QString& path = QString(), i32 startSecond = -1);
    inline void showAboutWindow();
    inline void processDroppedFiles(const QStringList& files);
    inline void
    updateProgressLabel(u32 second, const QString& duration = QString());
    inline void updateVolume(u8 value);
    inline void cancelSearchInput();
    inline void toggleEqualizerMenu();
    inline void exit();
    inline void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    inline void setupTrayIcon();
    inline auto changePlaylist(i8 index) -> bool;
    inline void closeTab(i8 index);
    inline void resetSorting(i32 index, Qt::SortOrder sortOrder);
    inline void retranslate(QLocale::Language language = QLocale::AnyLanguage);
    inline void moveDockWidget(DockWidget::Position dockWidgetPosition);
    inline void processArgs(const QStringList& args);
    inline void focus();
    inline void importPlaylist(bool createNewTab, QString filePath = QString());
    inline void exportPlaylist();
    inline void adjustPlaylistView();
    inline void adjustPlaylistTabBar(
        DockWidget::Position dockWidgetPosition,
        u8 currentIndex
    );
    inline void adjustPlaylistImage(
        DockWidget::Position dockWidgetPosition,
        u8 currentIndex
    );
    inline void handleConnectionIPC();
    inline void adjustPlayingPlaylist(
        PlaylistView::TabRemoveMode mode,
        u8 startIndex,
        u8 count
    );
    inline void showVisualizer();
    inline void showRepeatButtonContextMenu(const QPoint& pos);
    inline void showControlContainerContextMenu();

    inline void exportPlaylist(PlaylistFileType playlistType);

    static inline auto constructAudioFileFilter() -> QString;

    inline void reacquireSpectrumVisualizer();

    inline auto setupUi() -> Ui::MainWindow*;

    inline void checkForUpdates(bool manual = false);

    inline void updateStatusBar();
    inline void toggleRepeat();

    static constexpr u8 SEARCH_INPUT_POSITION_PADDING = 16;
    static constexpr QMargins SEARCH_INPUT_TEXT_MARGINS = { 2, 2, 2, 2 };
    static constexpr u16 SEARCH_INPUT_MIN_WIDTH = 256;
    static constexpr u8 SEARCH_INPUT_HEIGHT = 40;

    QList<QModelIndex> searchMatches;

    const QString IPCServerName;
    QString previousSearchPrompt;

    shared_ptr<Settings> settings;

    isize searchMatchesPosition;

    // UI
    Ui::MainWindow* const ui;

    // Status Bar
    // TODO: Refactor into `StatusBar` class
    QStatusBar* const statusBar;
    QLabel* const playlistTracksStatus;
    QLabel* const playlistDurationStatus;
    QLabel* const currentTrackStatus;

    // Tray
    QSystemTrayIcon* const trayIcon;
    OptionMenu* const trayIconMenu;
    ProgressLabel* const progressLabelTray;
    QLabel* const volumeLabelTray;
    CustomSlider* const volumeSliderTray;
    CustomSlider* const progressSliderTray;

    // Labels
    ProgressLabel* const progressLabel;
    QLabel* const volumeLabel;
    QLabel* const trackLabel;

    // Playlist
    PlaylistView* const playlistView;
    PlaylistTabBar* const playlistTabBar;

    // Buttons
    ActionButton* const playButton;
    ActionButton* const stopButton;
    ActionButton* const backwardButton;
    ActionButton* const forwardButton;
    IconTextButton* const repeatButton;
    ActionButton* const randomButton;
    QPushButton* const equalizerButton;

    // Sliders
    CustomSlider* const volumeSlider;
    CustomSlider* const progressSlider;

    // Icons
    const QIcon pauseIcon;
    const QIcon startIcon;

    // Actions
    const QAction* const actionOpenFile;
    const QAction* const actionOpenFolder;
    const QAction* const actionOpenPlaylist;
    const QAction* const actionSettings;
    const QAction* const actionVisualizer;
    QAction* actionExit;

    const QAction* const actionAddFile;
    const QAction* const actionAddFolder;
    const QAction* const actionAddPlaylist;

    const QAction* const actionExportXSPFPlaylist;
    const QAction* const actionExportM3U8Playlist;

    const QAction* const actionRussian;
    const QAction* const actionEnglish;

    const QAction* const actionAbout;
    const QAction* const actionDocumentation;
    const QAction* const actionCheckForUpdates;

    QAction* actionForward;
    QAction* actionBackward;
    QAction* actionRepeat;
    QAction* actionTogglePlayback;
    QAction* actionStop;
    QAction* actionRandom;

    QTranslator* translator;

    // Dock widgets
    QSplitter* const mainArea;
    DockWidget* const dockWidget;
    ScaledLabel* const dockCoverLabel;
    QTreeWidget* const dockMetadataTree;

    // Threads
    QThreadPool* const threadPool;

    // Audio
    SpectrumVisualizer* spectrumVisualizer;
    AudioWorker* audioWorker;

    // Search
    CustomInput* const searchTrackInput;
    const QShortcut* const searchShortcut;

    // Misc
    QLocalServer* const server;

    TrackRepeatMenu* const trackRepeatMenu;
    EqualizerMenu* equalizerMenu = nullptr;

    QProgressDialog* updateProgressDialog = nullptr;

#ifdef PROJECTM
    VisualizerDialog* visualizerDialog = nullptr;
#endif

    i32 CUEOffset = -1;
};
