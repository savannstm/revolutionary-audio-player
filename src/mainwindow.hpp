#pragma once

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

#include "customslider.hpp"
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
    QByteArray* audioBytes;
    QBuffer* audioBuffer;
    QAudioSink* audioSink;
    string audioDuration;
    CustomSlider* progressSlider;
    QPlainTextEdit* progressTimer;
    QPushButton* playButton;
    QTreeView* trackTree;
    QHeaderView* trackTreeHeader;
    QStandardItemModel* trackModel;
    u32 audioBytesNum;
    static auto toMinutes(u64 secs) -> string;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    inline void fillTable(const path& filePath) const;
    inline void fillTable(const vector<path>& paths) const;
    inline void fillTable(const walk_dir& read_dir) const;
    inline void jumpToTrack(Direction direction);
    inline void updatePosition(u32 pos) const;
    void checkPosition();

   signals:
    void filesDropped(vector<path> filePaths);
    void playbackFinished();

   private:
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
    QTimer timer;
    u32 lastPos;
    u32 previousPosition;
    u32 previousSecond;
    QThreadPool* threadPool;
};
