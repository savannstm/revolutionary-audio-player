#pragma once

#include "ui_settingswindow.h"

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui {
    class SettingsWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Implement settings window

class SettingsWindow : public QDialog {
    Q_OBJECT

   public:
    explicit SettingsWindow(QWidget* parent);
    ~SettingsWindow() override;

   private:
    auto setupUi() -> bool;

    Ui::SettingsWindow* ui = new Ui::SettingsWindow();
    const bool g = setupUi();
};