#include "SettingsWindow.hpp"

#include "Associations.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"

#include <QFileDialog>
#include <QStyleFactory>
#include <QStyleHints>

SettingsWindow::SettingsWindow(
    shared_ptr<Settings> settings_,
    QWidget* const parent
) :
    QDialog(parent),
    settings(std::move(settings_)) {
    this->adjustSize();

    for (const QString& style : QStyleFactory::keys()) {
        styleSelect->addItem(style);
    }

    if (!settings->core.applicationStyle.isEmpty()) {
        for (const u8 idx : range(0, styleSelect->count())) {
            if (settings->core.applicationStyle == styleSelect->itemText(idx)) {
                styleSelect->setCurrentText(settings->core.applicationStyle);
            }
        }
    }

    fetchDevices();

    settingsDirectoryInput->setText(settings->core.settingsDir);

    const QAction* const locateSettingsDirAction =
        settingsDirectoryInput->addAction(
            QIcon::fromTheme(QIcon::ThemeIcon::EditFind),
            QLineEdit::TrailingPosition
        );

    playlistDirectoryInput->setText(settings->core.playlistsDir);

    const QAction* const locatePlaylistDirAction =
        playlistDirectoryInput->addAction(
            QIcon::fromTheme(QIcon::ThemeIcon::EditFind),
            QLineEdit::TrailingPosition
        );

    createMenuItemCheckbox->setChecked(settings->shell.contextMenuEntryEnabled);
    for (const auto [idx, ext] :
         views::enumerate(ALLOWED_PLAYABLE_EXTENSIONS)) {
        enabledAssociationsList->addItem(ext.toString());

        enabledAssociationsList->item(enabledAssociationsList->count() - 1)
            ->setCheckState(
                ((u32(settings->shell.enabledAssociations) >> idx) & 1) != 0
                    ? Qt::Checked
                    : Qt::Unchecked
            );
    }

    // Playlist
    playlistNamingSelect->setCurrentIndex(
        u8(settings->playlist.playlistNaming)
    );

    backgroundImageCheckbox->setChecked(settings->playlist.autoSetBackground);
    prioritizeExternalCheckbox->setChecked(
        settings->playlist.prioritizeExternalCover
    );

    setTrackPropertiesLabels();

    connect(
        themeSelect,
        &QComboBox::currentIndexChanged,
        this,
        &SettingsWindow::changeTheme
    );

    connect(
        locateSettingsDirAction,
        &QAction::triggered,
        this,
        &SettingsWindow::locateSettingsDirectory
    );

    connect(
        locatePlaylistDirAction,
        &QAction::triggered,
        this,
        &SettingsWindow::locatePlaylistDirectory
    );

    connect(
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        &SettingsWindow::changeStyle
    );

    connect(
        enableAllAudioButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::enableAllAudio
    );

    connect(
        enableAllVideoButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::enableAllVideo
    );

    connect(
        enableAllPlaylistsButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::enableAllPlaylists
    );

    connect(
        enableAllButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::enableAllAssociations
    );

    connect(
        clearAllButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::clearAllAssociations
    );

    connect(
        setButton,
        &QPushButton::pressed,
        this,
        &SettingsWindow::setAssociations
    );

    connect(
        createMenuItemCheckbox,
        &QCheckBox::toggled,
        this,
        &SettingsWindow::setContextMenuEntry
    );

    connect(
        outputDeviceSelect,
        &QComboBox::currentIndexChanged,
        this,
        &SettingsWindow::onOutputDeviceChanged
    );

    connect(
        prioritizeExternalCheckbox,
        &QCheckBox::toggled,
        this,
        &SettingsWindow::togglePrioritizeExternal
    );

    connect(
        backgroundImageCheckbox,
        &QCheckBox::toggled,
        this,
        &SettingsWindow::toggleBackgroundImage
    );

    connect(
        playlistNamingSelect,
        &QComboBox::currentIndexChanged,
        this,
        &SettingsWindow::changePlaylistNaming
    );

    connect(
        settingsList,
        &QListWidget::currentRowChanged,
        this,
        &SettingsWindow::changeSettingsSection
    );

    connect(
        deviceMonitor,
        &DeviceMonitor::deviceAdded,
        this,
        &SettingsWindow::onDeviceAdded
    );

    connect(
        deviceMonitor,
        &DeviceMonitor::deviceRemoved,
        this,
        &SettingsWindow::onDeviceRemoved
    );
}

SettingsWindow::~SettingsWindow() {
    ma_context_uninit(&context);

    for (const u8 idx : range(0, columnList->count())) {
        if (columnList->item(idx)->checkState() == Qt::Checked) {
            settings->playlist.defaultColumns[idx] = TrackProperty(
                columnList->item(idx)->data(PROPERTY_ROLE).toUInt()
            );
        } else {
            settings->playlist.defaultColumns[idx] = TrackProperty::Play;
        }
    }

    const QString settingsDir = settingsDirectoryInput->text();

    if (QFile::exists(settingsDir)) {
        settings->core.settingsDir = settingsDir;
    }

    const QString playlistsDir = playlistDirectoryInput->text();

    if (QFile::exists(playlistsDir)) {
        settings->core.playlistsDir = playlistsDir;
    }

    delete ui;
}

void SettingsWindow::fetchDevices() {
    ma_result result = ma_context_init(nullptr, -1, &config, &context);
    if (result != MA_SUCCESS) {
        LOG_ERROR(
            u"Failed to fetch output devices: "_s +
            ma_result_description(result)
        );
        return;
    }

    ma_device_info* playbackInfos = nullptr;
    u32 playbackCount = 0;

    result = ma_context_get_devices(
        &context,
        &playbackInfos,
        &playbackCount,
        nullptr,
        nullptr
    );

    if (result != MA_SUCCESS) {
        LOG_ERROR(
            u"Failed to fetch output devices: "_s +
            ma_result_description(result)
        );
        return;
    }

    playbackDevices = span<ma_device_info>(playbackInfos, playbackCount);

    for (const auto& device : playbackDevices) {
        outputDeviceSelect->addItem(device.name);

        if (device.name == settings->core.outputDevice) {
            outputDeviceSelect->setCurrentIndex(
                outputDeviceSelect->count() - 1
            );
        }
    }

    if (settings->core.outputDevice.isEmpty()) {
        outputDeviceSelect->setCurrentIndex(0);
    };
}

void SettingsWindow::setTheme(const Qt::ColorScheme colorScheme) {
    QApplication::styleHints()->setColorScheme(colorScheme);
}

void SettingsWindow::changeTheme(const u8 state) {
    settings->core.applicationTheme = Qt::ColorScheme(state);
    setTheme(Qt::ColorScheme(state));
}

void SettingsWindow::locateSettingsDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(this);
    if (!dir.isEmpty()) {
        settingsDirectoryInput->setText(dir);
    }
}

void SettingsWindow::locatePlaylistDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(this);
    if (!dir.isEmpty()) {
        playlistDirectoryInput->setText(dir);
    }
}

void SettingsWindow::changeStyle(const QString& itemText) {
    settings->core.applicationStyle = itemText;

    if (styleSelect->currentIndex() == 0) {
        QApplication::setStyle(settings->core.defaultStyle);
    } else {
        QApplication::setStyle(itemText);
    }
}

void SettingsWindow::enableAllAudio() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                ALLOWED_AUDIO_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllVideo() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                ALLOWED_VIDEO_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllPlaylists() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                ALLOWED_PLAYLIST_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllAssociations() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
    }
}

void SettingsWindow::clearAllAssociations() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        enabledAssociationsList->item(idx)->setCheckState(Qt::Unchecked);
    }
}

void SettingsWindow::setAssociations() {
    for (u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
        if (enabledAssociationsList->item(idx)->checkState() == Qt::Checked) {
            settings->shell.enabledAssociations |= Associations(1 << idx);
        } else {
            settings->shell.enabledAssociations &= Associations(~(1 << idx));
        }
    }

    updateFileAssociations(settings->shell.enabledAssociations);
}

void SettingsWindow::setContextMenuEntry(const bool checked) {
    if (checked) {
        createContextMenuEntry();
    } else {
        removeContextMenuEntry();
    }

    settings->shell.contextMenuEntryEnabled = checked;
}

void SettingsWindow::changePlaylistNaming(const u8 index) {
    settings->playlist.playlistNaming = PlaylistNaming(index);
}

void SettingsWindow::togglePrioritizeExternal(const bool checked) {
    settings->playlist.prioritizeExternalCover = checked;
}

void SettingsWindow::toggleBackgroundImage(const bool checked) {
    settings->playlist.autoSetBackground = checked;
}

void SettingsWindow::changeSettingsSection(const u16 row) {
    stackedWidget->setCurrentIndex(row);
}

void SettingsWindow::onDeviceAdded(const QString& deviceName) {
    for (u8 idx : range(1, outputDeviceSelect->count())) {
        if (outputDeviceSelect->itemText(idx) == deviceName) {
            return;
        }
    }
    outputDeviceSelect->addItem(deviceName);
}

void SettingsWindow::onDeviceRemoved(const QString& deviceName) {
    for (u8 idx : range(1, outputDeviceSelect->count())) {
        if (outputDeviceSelect->itemText(idx) == deviceName) {
            outputDeviceSelect->removeItem(idx);
            break;
        }
    }
}

void SettingsWindow::onOutputDeviceChanged(const u8 index) {
    if (index == outputDeviceSelect->count() - 1) {
        settings->core.outputDevice.clear();
        settings->core.outputDeviceID = std::nullopt;
    } else {
        settings->core.outputDevice = playbackDevices[index].name;
        settings->core.outputDeviceID = playbackDevices[index].id;
    }

    emit audioDeviceChanged();
}
