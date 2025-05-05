#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audioworker.hpp"
#include "constants.hpp"
#include "custominput.hpp"
#include "customslider.hpp"
#include "equalizermenu.hpp"
#include "icontextbutton.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"
#include "optionmenu.hpp"
#include "playlistview.hpp"
#include "scaledlabel.hpp"
#include "settings.hpp"
#include "tracktree.hpp"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
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
// TODO: Wave visualizer
// TODO: More Playlist / File menu sections buttons

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
    void trackDataReady(
        TrackTree* tree,
        const metadata_array& metadata,
        const QString& path
    );
    void trackDataProcessed(TrackTree* tree);
    void retranslated();

   private:
    [[nodiscard]] inline auto saveSettings() -> result<bool, QString>;
    inline void loadSettings();
    [[nodiscard]] inline auto setupUi() -> Ui::MainWindow*;
    inline void jumpToTrack(Direction direction, bool clicked);
    inline void updatePlaybackPosition();
    inline void playTrack(TrackTree* tree, const QModelIndex& index);
    inline void stopPlayback();
    inline void handleTrackPress(const QModelIndex& index);
    inline void handleHeaderPress(u8 index, Qt::MouseButton button);
    inline void searchTrack();
    inline void showSearchInput();
    inline void onAudioProgressUpdated(u16 second);
    inline void playNext();
    inline void updateProgress(u16 seconds);
    inline void showSettingsWindow();
    inline void showHelpWindow();
    inline void addEntry(bool createNewTab, bool isFolder);
    inline void togglePlayback();
    inline void moveForward();
    inline void moveBackward();
    inline void showAboutWindow();
    inline void processDroppedFiles(const QStringList& files);
    inline void updateProgressLabel();
    inline void updateVolume(u16 value);
    inline void cancelSearchInput();
    inline void toggleEqualizerMenu(bool checked);
    inline void jumpForward();
    inline void jumpBackward();
    inline void toggleRepeat();
    inline void pausePlayback();
    inline void resumePlayback();
    inline void toggleRandom();
    inline void exit();
    inline void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    inline void setupTrayIcon();
    inline void changePlaylist(i8 index);
    inline void closeTab(i8 index);
    inline void selectTrack(i32 oldRow, u16 newRow);
    inline void resetSorting(i32 index, Qt::SortOrder sortOrder);
    inline void retranslate(QLocale::Language language = QLocale::AnyLanguage);
    inline void moveDockWidget(DockWidgetPosition dockWidgetPosition);

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
    TrackTree* trackTree = nullptr;
    MusicHeader* trackTreeHeader = nullptr;
    MusicModel* trackTreeModel = nullptr;

    // UI - Buttons
    QPushButton* playButton = ui->playButton;
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
    QAction* actionExit = ui->actionExit;
    QAction* actionOpenFile = ui->actionOpenFile;
    QAction* actionOpenFolder = ui->actionOpenFolder;
    QAction* actionAbout = ui->actionAbout;
    QAction* actionForward = ui->actionForward;
    QAction* actionBackward = ui->actionBackward;
    QAction* actionRepeat = ui->actionRepeat;
    QAction* actionPause = ui->actionPause;
    QAction* actionResume = ui->actionResume;
    QAction* actionStop = ui->actionStop;
    QAction* actionRandom = ui->actionRandom;
    QAction* actionSettings = ui->actionSettings;
    QAction* actionHelp = ui->actionHelp;
    QAction* actionAddFile = ui->actionAddFile;
    QAction* actionAddFolder = ui->actionAddFolder;
    QAction* actionRussian = ui->actionRussian;
    QAction* actionEnglish = ui->actionEnglish;

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
    QSplitter* dockWidget = ui->dockWidget;
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
        QApplication::applicationDirPath() + "/rap-settings.json";

    // Miscellaneous
    EqualizerMenu* equalizerMenu = nullptr;
    u8 sortIndicatorCleared;
    std::random_device rng;
};
