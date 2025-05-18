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

class SettingsWindow : public QDialog {
    Q_OBJECT

   public:
    explicit SettingsWindow(shared_ptr<Settings> settings, QWidget* parent);
    ~SettingsWindow() override;

   signals:
    void audioDeviceChanged(const QAudioDevice& device);

   private:
    static void removeOpenDirectoryEntry();
    static void addOpenDirectoryEntry();
    static void createFileAssociations();
    static void removeFileAssociations();
    auto setupUi() -> Ui::SettingsWindow*;

    Ui::SettingsWindow* ui = setupUi();

    shared_ptr<Settings> settings;

    QComboBox* styleSelect = ui->styleSelect;
    QComboBox* playlistNamingSelect = ui->playlistNamingSelect;
    QComboBox* dragDropSelect = ui->dragdropSelect;
    QCheckBox* createMenuItemCheckbox = ui->createMenuItemCheckbox;
    QCheckBox* setAssociationsCheckbox = ui->setAssociationsCheckbox;
    QComboBox* outputDeviceSelect = ui->outputDeviceSelect;

    const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
};