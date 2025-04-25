#pragma once

#include "ui_settingswindow.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>

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
    auto setupUi() -> Ui::SettingsWindow*;

    Ui::SettingsWindow* ui = setupUi();
    QComboBox* styleSelect = ui->styleSelect;
    QLabel* styleSelectLabel = ui->styleSelectLabel;
};