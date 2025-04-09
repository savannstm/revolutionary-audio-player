#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QAction>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QStandardItemModel>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTextEdit>

#include "type_aliases.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}  // namespace Ui
QT_END_NAMESPACE

static constexpr u8 sampleSize = sizeof(i16);
static constexpr array<cstr, 9> EXTENSIONS = {
    ".mp3", ".flac", ".opus", ".aac", ".wav", ".ogg", ".m4a", ".mp4", ".mkv"};

enum Direction : u8 { Forward, BackwardRandom, Backward, ForwardRandom };

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    static inline void fillTable(const path& filePath);
    static inline void fillTable(const vector<path>& paths);
    static inline void fillTable(const walk_dir& read_dir);
    static inline void jumpToTrack(Direction direction);
    static inline void updatePosition(u32 pos);

   signals:
    void filesDropped(vector<path> filePaths);

   private:
    Ui::MainWindow* ui;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
};

#endif  // MAINWINDOW_HPP
