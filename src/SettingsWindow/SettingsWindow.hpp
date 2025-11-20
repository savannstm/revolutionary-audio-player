#pragma once

#include "DeviceMonitor.hpp"
#include "Settings.hpp"
#include "TrackProperties.hpp"
#include "ui_SettingsWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>

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

    static void setTheme(Qt::ColorScheme colorScheme);

   signals:
    void audioDeviceChanged();

   protected:
    void changeEvent(QEvent* const event) override {
        if (event->type() == QEvent::LanguageChange) {
            ui->retranslateUi(this);
            setTrackPropertiesLabels();
            outputDeviceSelect->setItemText(0, tr("Default"));
        }

        QDialog::changeEvent(event);
    }

   private:
    void changeTheme(u8 state);
    void locateSettingsDirectory();
    void locatePlaylistDirectory();
    void changeStyle(const QString& itemText);

    void enableAllAudio();
    void enableAllVideo();
    void enableAllPlaylists();
    void enableAllAssociations();
    void clearAllAssociations();
    void setAssociations();
    void setContextMenuEntry(bool checked);

    void changePlaylistNaming(u8 index);
    void togglePrioritizeExternal(bool checked);
    void toggleBackgroundImage(bool checked);

    void changeSettingsSection(u16 row);

    void onDeviceAdded(const QString& deviceName);
    void onDeviceRemoved(const QString& deviceName);

    void onOutputDeviceChanged(u8 index);

    auto setupUi() -> Ui::SettingsWindow* {
        auto* const ui_ = new Ui::SettingsWindow();
        ui_->setupUi(this);
        return ui_;
    };

    inline void fetchDevices();

    void setTrackPropertiesLabels() {
        columnList->clear();
        const QStringList labels = trackPropertiesLabels();

        for (const u8 property : range(1, TRACK_PROPERTY_COUNT)) {
            columnList->addItem(labels[property]);
            columnList->item(columnList->count() - 1)
                ->setData(PROPERTY_ROLE, property);
            columnList->item(columnList->count() - 1)
                ->setCheckState(
                    ranges::contains(
                        settings->playlist.defaultColumns,
                        TrackProperty(property)
                    )
                        ? Qt::Checked
                        : Qt::Unchecked
                );
        }
    }

    ma_context context;
    const ma_context_config config = ma_context_config_init();

    span<ma_device_info> playbackDevices;

    shared_ptr<Settings> settings;

    DeviceMonitor* const deviceMonitor = new DeviceMonitor(this);
    Ui::SettingsWindow* const ui = setupUi();

    QStackedWidget* const stackedWidget = ui->stackedWidget;
    QListWidget* const settingsList = ui->settingsList;

    // Core
    QLineEdit* const settingsDirectoryInput = ui->settingsDirInput;
    QLineEdit* const playlistDirectoryInput = ui->playlistDirInput;
    QComboBox* const themeSelect = ui->themeSelect;
    QComboBox* const styleSelect = ui->styleSelect;
    QComboBox* const outputDeviceSelect = ui->outputDeviceSelect;

    // Shell
    QCheckBox* const createMenuItemCheckbox = ui->createMenuItemCheckbox;
    QListWidget* const enabledAssociationsList = ui->enabledAssociationsList;

    QPushButton* const enableAllAudioButton = ui->enableAllAudioButton;
    QPushButton* const enableAllVideoButton = ui->enableAllVideoButton;
    QPushButton* const enableAllPlaylistsButton = ui->enableAllPlaylistsButton;
    QPushButton* const enableAllButton = ui->enableAllButton;
    QPushButton* const clearAllButton = ui->clearAllButton;
    QPushButton* const setButton = ui->setButton;

    // Playlists
    QCheckBox* const prioritizeExternalCheckbox =
        ui->prioritizeExternalCheckbox;
    QCheckBox* const backgroundImageCheckbox = ui->backgroundImageCheckbox;

    QComboBox* const playlistNamingSelect = ui->playlistNamingSelect;

    QListWidget* const columnList = ui->columnList;
};