#pragma once

#include <QDialog>
#include <QMainWindow>

#include "ui_aboutwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AboutWindow;
}  // namespace Ui
QT_END_NAMESPACE

class AboutWindow : public QDialog {
    Q_OBJECT

   public:
    AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow() override;

   private:
    Ui::AboutWindow* ui;
};
