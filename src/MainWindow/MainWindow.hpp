#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FWD.hpp"
#include "IndexSet.hpp"

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
    inline void advancePlaylist(Direction direction);
    inline void updatePlaybackPosition();
    inline void playTrack(TrackTree* tree, const QModelIndex& index);
    inline void stopPlayback();
    inline void handleTrackPress(const QModelIndex& index);
    inline void searchTrack();
    inline void toggleSearchInput(bool forceShow = false);
    inline void onAudioProgressUpdated(u16 second);
    inline void playNext();
    inline void updateProgress(u16 seconds);
    inline void showSettingsWindow();
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
    inline void handleConnectionIPC();
    inline void
    adjustPlayingPlaylist(TabRemoveMode mode, u8 startIndex, u8 count);
    inline void showVisualizer();
    inline void showRepeatButtonContextMenu(const QPoint& pos);
    inline void showControlContainerContextMenu();

    inline void exportPlaylist(PlaylistFileType playlistType);

    static inline auto constructAudioFileFilter() -> QString;

    inline void reacquireSpectrumVisualizer();

    inline auto setupUi() -> Ui::MainWindow*;

    // UI
    Ui::MainWindow* const ui;

    // Tray
    QSystemTrayIcon* const trayIcon;
    OptionMenu* const trayIconMenu;
    QLabel* const progressLabelTray;
    QLabel* const volumeLabelTray;
    CustomSlider* const volumeSliderTray;
    CustomSlider* const progressSliderTray;

    // Labels
    QLabel* const progressLabel;
    QLabel* const volumeLabel;
    QLabel* const trackLabel;

    // Playlist
    PlaylistView* const playlistView;
    PlaylistTabBar* const playlistTabBar;
    TrackTree* trackTree;
    TrackTreeHeader* trackTreeHeader;
    TrackTreeModel* trackTreeModel;

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

    QAction* actionForward;
    QAction* actionBackward;
    QAction* actionRepeat;
    QAction* actionTogglePlayback;
    QAction* actionStop;
    QAction* actionRandom;

    // Settings
    shared_ptr<Settings> settings;

    // UI State
    const QString ZERO_DURATION;
    const QString FULL_ZERO_DURATION;
    const QString IPCServerName;

    QString trackDuration;
    QTranslator* translator;

    // Control Logic
    IndexSet playHistory;

    bool random;
    RepeatMode repeatMode;
    Direction forwardDirection;
    Direction backwardDirection;
    i8 playingPlaylist;

    // Dock widgets
    QSplitter* const mainArea;
    DockWidget* const dockWidget;
    ScaledLabel* const dockCoverLabel;
    QTreeWidget* const dockMetadataTree;

    // Threads
    QThreadPool* const threadPool;

    // Audio
    array<f32, MIN_BUFFER_SIZE / F32_SAMPLE_SIZE> visualizerBuffer;
    array<f32, MIN_BUFFER_SIZE / F32_SAMPLE_SIZE> spectrumVisualizerBuffer;

    SpectrumVisualizer* spectrumVisualizer;
    AudioWorker* audioWorker;

    // Search
    CustomInput* const searchTrackInput;
    const QShortcut* const searchShortcut;

    QVector<QModelIndex> searchMatches;
    isize searchMatchesPosition;

    QString previousSearchPrompt;

    // Misc
    QString currentTrack;

    QLocalServer* const server;

    TrackRepeatMenu* const trackRepeatMenu;
    EqualizerMenu* equalizerMenu = nullptr;

#ifdef PROJECTM
    VisualizerDialog* visualizerDialog = nullptr;
#endif

    u16 CUEOffset = UINT16_MAX;
};
