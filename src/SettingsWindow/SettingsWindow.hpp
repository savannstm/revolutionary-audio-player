#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>
#include <miniaudio.h>

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
    void changeEvent(QEvent* event) override;

   private:
    inline void changeTheme(u8 state);
    inline void locateSettingsDirectory();
    inline void locatePlaylistDirectory();
    inline void changeStyle(const QString& itemText);

    inline void enableAllAudio();
    inline void enableAllVideo();
    inline void enableAllPlaylists();
    inline void enableAllAssociations();
    inline void clearAllAssociations();
    inline void setAssociations();
    inline void setContextMenuEntry(bool checked);

    inline void changePlaylistNaming(u8 index);
    inline void togglePrioritizeExternal(bool checked);
    inline void toggleBackgroundImage(bool checked);

    inline void changeSettingsSection(u16 row);

    inline void onDeviceAdded(const QString& deviceName);
    inline void onDeviceRemoved(const QString& deviceName);

    inline void onOutputDeviceChanged(u8 index);

    inline auto setupUi() -> Ui::SettingsWindow*;

    inline void fetchDevices();

    inline void setTrackPropertiesLabels();

    ma_context context;
    const ma_context_config config = ma_context_config_init();

    span<ma_device_info> playbackDevices;

    shared_ptr<Settings> settings;

    Ui::SettingsWindow* const ui;

    QStackedWidget* const stackedWidget;
    QListWidget* const settingsList;

    // Core
    QLineEdit* const settingsDirectoryInput;
    QLineEdit* const playlistDirectoryInput;
    QComboBox* const themeSelect;
    QComboBox* const styleSelect;
    QComboBox* const outputDeviceSelect;

    // Shell
    QCheckBox* const createMenuItemCheckbox;
    QListWidget* const enabledAssociationsList;

    QPushButton* const enableAllAudioButton;
    QPushButton* const enableAllVideoButton;
    QPushButton* const enableAllPlaylistsButton;
    QPushButton* const enableAllButton;
    QPushButton* const clearAllButton;
    QPushButton* const setButton;

    // Playlists
    QCheckBox* const prioritizeExternalCheckbox;

    QCheckBox* const backgroundImageCheckbox;

    QComboBox* const playlistNamingSelect;

    QListWidget* const columnList;

    DeviceMonitor* const deviceMonitor;
};