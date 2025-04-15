#pragma once

#include "actionbutton.hpp"
#include "aliases.hpp"
#include "audiostreamer.hpp"
#include "constants.hpp"
#include "customslider.hpp"
#include "indexset.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QAction>
#include <QAudioSink>
#include <QCloseEvent>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTreeView>
#include <random>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}  // namespace Ui

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    void updateProgress();
    void eof();

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void filesDropped(const vector<string>& filePaths);
    void trackDataReady(
        const array<string, PROPERTY_COUNT>& metadata,
        const string& path
    );
    void trackDataProcessed();

   private:
    // UI
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    QMenu* trayIconMenu = new QMenu(this);
    QPushButton* playButton;
    QTreeView* trackTree;
    MusicHeader* trackTreeHeader;
    MusicModel* trackModel = new MusicModel(this);
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");
    QSettings* settings =
        new QSettings("com.savannstm.rap", "com.savannstm.rap");

    QString lastDir =
        settings->value("lastOpenedDir", QDir::homePath()).toString();

    QModelIndex currentIndex;

    CustomSlider* volumeSlider;
    QPushButton* stopButton;
    QPushButton* backwardButton;
    QPushButton* forwardButton;
    ActionButton* repeatButton;
    ActionButton* randomButton;

    QMenu* fileMenu;
    QAction* exitAction;
    QAction* openFileAction;
    QAction* openFolderAction;
    QAction* aboutAction;
    QAction* forwardAction;
    QAction* backwardAction;
    QAction* repeatAction;
    QAction* pauseAction;
    QAction* resumeAction;
    QAction* stopAction;
    QAction* randomAction;

    CustomSlider* progressSlider;
    QLabel* progressLabel;

    QStringList headerLabels = { "Title", "Artist", "Track Number" };

    // Play history
    IndexSet<u32> playHistory = IndexSet<u32>();

    // Controls
    RepeatMode repeat = RepeatMode::Off;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;

    f64 audioVolume = 1.0;
    QLabel* volumeLabel;

    AudioStreamer audioStreamer = AudioStreamer(this);
    QAudioSink* audioSink = new QAudioSink();

    // Timer
    string audioDuration = "0:00";

    // Thread pool
    QThreadPool* threadPool;

    // Eq
    bool eqEnabled = false;
    array<f32, EQ_BANDS_N> eqGains = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    std::random_device randdevice;

    QLineEdit* searchTrackInput = new QLineEdit(this);
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);

    inline void fillTable(const string& filePath);
    inline void fillTable(const vector<string>& paths);
    inline void fillTable(const walk_dir& read_dir);
    inline void jumpToTrack(Direction direction, bool clicked);
    inline void updatePlaybackPosition();
    inline void playTrack(const string& path);
    inline void stopPlayback();
};
