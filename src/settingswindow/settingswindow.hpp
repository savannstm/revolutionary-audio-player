#pragma once

#include "settings.hpp"
#include "ui_settingswindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>

QT_BEGIN_NAMESPACE

namespace Ui {
    class SettingsWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Implement settings window
// TODO: Implement changeable output

class SettingsWindow : public QDialog {
    Q_OBJECT

   public:
    explicit SettingsWindow(shared_ptr<Settings> settings, QWidget* parent);
    ~SettingsWindow() override;

   private:
    static void removeOpenDirectoryEntry();
    static void addOpenDirectoryEntry();
    static void associateFileTypes();
    auto setupUi() -> Ui::SettingsWindow*;
    Ui::SettingsWindow* ui = setupUi();

    shared_ptr<Settings> settings;

    QComboBox* styleSelect = ui->styleSelect;
    QLabel* styleSelectLabel = ui->styleSelectLabel;
    QComboBox* playlistNamingSelect = ui->playlistNamingSelect;
    QLabel* playlistNamingLabel = ui->playlistNamingLabel;
    QComboBox* dragDropSelect = ui->dragdropSelect;
    QLabel* dragDropSelectLabel = ui->dragdropSelectLabel;
    QCheckBox* createMenuItemCheckbox = ui->createMenuItemCheckbox;
};