#pragma once

#include "Settings.hpp"
#include "TrackProperties.hpp"
#include "ui_SettingsWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>

QT_BEGIN_NAMESPACE

namespace Ui {
    class SettingsWindow;
}  // namespace Ui

QT_END_NAMESPACE

// TODO: Add hot-reload of available output devices

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
            outputDeviceSelect->setItemText(
                outputDeviceSelect->count() - 1,
                tr("Default Device")
            );
        }

        QDialog::changeEvent(event);
    }

   private:
    auto setupUi() -> Ui::SettingsWindow* {
        auto* const ui_ = new Ui::SettingsWindow();
        ui_->setupUi(this);
        return ui_;
    };

    inline auto fetchDevices() -> ma_result;

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

    Ui::SettingsWindow* const ui = setupUi();

    shared_ptr<Settings> settings;

    ma_context context;
    const ma_context_config config = ma_context_config_init();
    span<ma_device_info> playbackDevices;

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