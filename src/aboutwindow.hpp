#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>
#include <QMainWindow>

#include "./ui_aboutwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AboutWindow;
}  // namespace Ui
QT_END_NAMESPACE

class AboutWindow : public QMainWindow {
    Q_OBJECT

   public:
    AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow() override;

   private:
    Ui::AboutWindow* ui;
};
#endif  // ABOUTWINDOW_H