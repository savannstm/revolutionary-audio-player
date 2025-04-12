#pragma once

#include "aliases.hpp"
#include "audiostreamer.hpp"
#include "customslider.hpp"
#include "indexset.hpp"

#include <QAction>
#include <QAudioSink>
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTimer>
#include <QTreeView>
#include <random>

QT_BEGIN_NAMESPACE

namespace Ui {
    class MainWindow;
}  // namespace Ui

QT_END_NAMESPACE

enum Direction : u8 {
    Forward,
    BackwardRandom,
    Backward,
    ForwardRandom
};

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    static inline auto toMinutes(u16 secs) -> string;
    void updateProgress();

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void filesDropped(vector<path> filePaths);

   private:
    // UI
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    QMenu* trayIconMenu;
    QPushButton* playButton;
    QTreeView* trackTree;
    QHeaderView* trackTreeHeader;
    QStandardItemModel* trackModel;
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");

    CustomSlider* volumeSlider;
    QPushButton* stopButton;
    QPushButton* backwardButton;
    QPushButton* forwardButton;
    QPushButton* repeatButton;
    QPushButton* randomButton;

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

    // Play history
    std::random_device rng;
    std::mt19937 gen;
    IndexSet<u32> playHistory = IndexSet<u32>();

    // Controls
    bool repeat = false;
    bool random = false;
    Direction forwardDirection = Direction::Forward;
    Direction backwardDirection = Direction::Backward;

    f64 audioVolume = 1.0;
    QLabel* volumeLabel;

    ref<AudioStreamer> audioStreamer;
    ref<QAudioSink> audioSink = make_unique<QAudioSink>();
    u32 audioBytesNum = 0;

    // Timer
    string audioDuration = "0:00";

    // Thread pool
    QThreadPool threadPool;

    inline void fillTable(const path& filePath) const;
    inline void fillTable(const vector<path>& paths) const;
    inline void fillTable(const walk_dir& read_dir) const;
    inline void jumpToTrack(Direction direction);
    inline void updatePlaybackPosition();
    inline void playTrack(const path& path);
    inline void stopPlayback();
};
