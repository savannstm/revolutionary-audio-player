#pragma once

#include <qplaintextedit.h>
#include <qpushbutton.h>

#include <QAction>
#include <QAudioSink>
#include <QBuffer>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QThreadPool>
#include <QTimer>
#include <QTreeView>
#include <random>

#include "customslider.hpp"
#include "indexset.hpp"
#include "type_aliases.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

enum Direction : u8 { Forward, BackwardRandom, Backward, ForwardRandom };

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Audio
    QByteArray* audioBytes;
    QBuffer* audioBuffer;
    QAudioSink* audioSink;
    u32 audioBytesNum;
    static inline auto toMinutes(u16 secs) -> string;

    // Timer
    string audioDuration = "0:00";

    // UI
    CustomSlider* progressSlider;
    QPlainTextEdit* progressTimer;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    inline void fillTable(const path& filePath) const;
    inline void fillTable(const vector<path>& paths) const;
    inline void fillTable(const walk_dir& read_dir) const;
    inline void jumpToTrack(Direction direction);
    inline void updatePlaybackPosition(u32 pos) const;
    inline void updatePositionDisplay();

   signals:
    void filesDropped(vector<path> filePaths);
    void playbackFinished();

   private:
    // UI
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QPushButton* playButton;
    QTreeView* trackTree;
    QHeaderView* trackTreeHeader;
    QStandardItemModel* trackModel;
    QIcon pauseIcon = QIcon::fromTheme("media-playback-pause");
    QIcon startIcon = QIcon::fromTheme("media-playback-start");

    // Play history
    std::random_device rng;
    std::mt19937 gen;
    IndexSet<u32>* playHistory;

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

    bool repeat;
    bool random;
    Direction forwardDirection;
    Direction backwardDirection;

    f64 audioVolume = 1.0;
    QPlainTextEdit* volumeLevel;

    // Timer
    QTimer timer;
    u32 lastPos;
    u32 previousPosition;
    u32 previousSecond;
    inline void stopPlayback();

    // Thread pool
    QThreadPool* threadPool;
};
