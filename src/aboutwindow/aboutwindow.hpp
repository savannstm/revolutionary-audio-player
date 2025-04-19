#pragma once

#include "ui_aboutwindow.h"

#include <QDialog>
#include <QMainWindow>

QT_BEGIN_NAMESPACE

namespace Ui {
    class AboutWindow;
}  // namespace Ui

QT_END_NAMESPACE

class AboutWindow : public QDialog {
    Q_OBJECT

   public:
    AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow() override = default;

   private:
    Ui::AboutWindow* ui;
};
