#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audioworker.hpp"
#include "custominput.hpp"
#include "customslider.hpp"
#include "equalizermenu.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"
#include "playlistview.hpp"
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

constexpr array<cstr, 9> ALLOWED_EXTENSIONS = { ".mp3", ".flac", ".opus",
                                                ".aac", ".wav",  ".ogg",
                                                ".m4a", ".mp4",  ".mkv" };

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

   private:
    void saveSettings();
    void loadSettings();
    auto setupUi() -> Ui::MainWindow*;

    // Inline utility functions
    inline void fillTable(TrackTree* tree, const QStringList& paths);
    inline void fillTable(TrackTree* tree, QDirIterator& iterator);
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
    inline void addFolder(bool createNewTab);
    inline void addFile(bool createNewTab);
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
    inline void initializeEqualizerMenu();
    inline void changePlaylist(i8 index);
    inline void closeTab(i8 index);
    inline void processFile(TrackTree* tree, const QString& filePath);
    inline void selectTrack(i32 oldRow, u16 newRow);
    inline auto getRowMetadata(TrackTree* tree, u16 row) -> QMap<u8, QString>;
    inline void resetSorting(i32 index, Qt::SortOrder sortOrder);

    // UI - Widgets
    Ui::MainWindow* ui = setupUi();
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
    QJsonObject settings;
    QString lastDir;

    // UI State
    QString audioDuration = "0:00";
    QTranslator translator;

    // Control Logic
    RepeatMode repeat = RepeatMode::Off;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;
    IndexSet playHistory;
    i8 playingPlaylist = -1;

    // Threads
    QThreadPool* threadPool = new QThreadPool();
    AudioWorker* audioWorker = new AudioWorker();

    // Search
    QVector<QModelIndex> searchMatches;
    isize searchMatchesPosition = 0;
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    QString previousSearchPrompt;

    // Miscellaneous
    std::random_device randdevice;
    QFile settingsFile =
        QApplication::applicationDirPath() + "/" + "rap-settings.json";
    EqualizerMenu* equalizerMenu = nullptr;
    u8 sortIndicatorCleared;
};
