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

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

   protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

   signals:
    void filesDropped(vector<path> filePaths);

   private:
    Ui::MainWindow* ui;
    QStandardItemModel* tracksModel;
    QSystemTrayIcon* trayIcon;
    QMenu* trayIconMenu;
};

#endif  // MAINWINDOW_HPP
