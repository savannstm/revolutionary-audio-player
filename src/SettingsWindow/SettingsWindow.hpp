#pragma once

#include "Settings.hpp"
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

// TODO: Add hot-reload of available output devices?
// TODO: Refactor settings into sections

class SettingsWindow : public QDialog {
    Q_OBJECT

   public:
    explicit SettingsWindow(shared_ptr<Settings> settings, QWidget* parent);
    ~SettingsWindow() override;

    static void removeContextMenuEntry();
    static void createContextMenuEntry();
    static void createFileAssociations();
    static void removeFileAssociations();

   signals:
    void audioDeviceChanged();

   private:
    auto setupUi() -> Ui::SettingsWindow*;
    inline auto fetchDevices() -> ma_result;

    Ui::SettingsWindow* ui = setupUi();

    shared_ptr<Settings> settings;

    ma_context context;
    ma_context_config config = ma_context_config_init();
    span<ma_device_info> playbackDevices;

    QComboBox* styleSelect = ui->styleSelect;
    QComboBox* playlistNamingSelect = ui->playlistNamingSelect;
    QCheckBox* createMenuItemCheckbox = ui->createMenuItemCheckbox;
    QCheckBox* setAssociationsCheckbox = ui->setAssociationsCheckbox;
    QComboBox* outputDeviceSelect = ui->outputDeviceSelect;
    QCheckBox* prioritizeExternalCheckbox = ui->prioritizeExternalCheckbox;
    QCheckBox* backgroundImageCheckbox = ui->backgroundImageCheckbox;
};