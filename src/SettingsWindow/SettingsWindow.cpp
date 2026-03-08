#include "SettingsWindow.hpp"

#include "Associations.hpp"
#include "Constants.hpp"
#include "DeviceMonitor.hpp"
#include "Enums.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "ui_SettingsWindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QListWidget>
#include <QStyleFactory>
#include <QStyleHints>

SettingsWindow::SettingsWindow(
    shared_ptr<Settings> settings_,
    QWidget* const parent
) :
    QDialog(parent),
    settings(std::move(settings_)),

    ui(setupUi()),
    stackedWidget(ui->stackedWidget),
    settingsList(ui->settingsList),

    settingsDirectoryInput(ui->settingsDirInput),
    playlistDirectoryInput(ui->playlistDirInput),
    themeSelect(ui->themeSelect),
    styleSelect(ui->styleSelect),
    outputDeviceSelect(ui->outputDeviceSelect),
    checkForUpdatesCheckbox(ui->checkForUpdatesCheckbox),

    createMenuItemCheckbox(ui->createMenuItemCheckbox),
    enabledAssociationsList(ui->enabledAssociationsList),
    enableAllAudioButton(ui->enableAllAudioButton),
    enableAllVideoButton(ui->enableAllVideoButton),
    enableAllPlaylistsButton(ui->enableAllPlaylistsButton),
    enableAllButton(ui->enableAllButton),
    clearAllButton(ui->clearAllButton),
    setButton(ui->setButton),

    prioritizeExternalCheckbox(ui->prioritizeExternalCheckbox),
    backgroundImageCheckbox(ui->backgroundImageCheckbox),
    playlistNamingSelect(ui->playlistNamingSelect),
    columnList(ui->columnList),

    deviceMonitor(new DeviceMonitor(this)) {
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
         views::enumerate(SUPPORTED_PLAYABLE_EXTENSIONS)) {
        enabledAssociationsList->addItem(ext.toString());

        enabledAssociationsList->item(enabledAssociationsList->count() - 1)
            ->setCheckState(
                (settings->shell.enabledAssociations &
                 Associations(1 << idx)) != Associations::None
                    ? Qt::Checked
                    : Qt::Unchecked
            );
    }

    checkForUpdatesCheckbox->setChecked(settings->core.checkForUpdates);

    // Playlist
    playlistNamingSelect->setCurrentIndex(
        u8(settings->playlist.playlistNaming)
    );

    backgroundImageCheckbox->setChecked(settings->playlist.autoSetBackground);
    prioritizeExternalCheckbox->setChecked(
        settings->playlist.prioritizeExternalCover
    );

    for (const auto prop : TRACK_PROPERTIES) {
        columnList->addItem(QString());
        columnList->item(u8(prop))->setData(Qt::UserRole, u8(prop));
    }

    setTrackPropertiesLabels();

    // Listeners
    connect(
        checkForUpdatesCheckbox,
        &QCheckBox::checkStateChanged,
        this,
        [this](const Qt::CheckState state) -> void {
        settings->core.checkForUpdates = state == Qt::Checked;
    }
    );

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
        settings->playlist.columns[idx].hidden =
            columnList->item(idx)->checkState() == Qt::Unchecked;
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
        qCritical() << "Failed to fetch output devices: "_L1
                    << ma_result_description(result);
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
        qCritical() << "Failed to fetch output devices: "_L1
                    << ma_result_description(result);
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
    qApp->styleHints()->setColorScheme(colorScheme);
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
        qApp->setStyle(settings->core.defaultStyle);
    } else {
        qApp->setStyle(itemText);
    }
}

void SettingsWindow::enableAllAudio() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                SUPPORTED_AUDIO_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllVideo() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                SUPPORTED_VIDEO_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllPlaylists() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        if (ranges::contains(
                SUPPORTED_PLAYLIST_EXTENSIONS,
                enabledAssociationsList->item(idx)->text()
            )) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    }
}

void SettingsWindow::enableAllAssociations() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
    }
}

void SettingsWindow::clearAllAssociations() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        enabledAssociationsList->item(idx)->setCheckState(Qt::Unchecked);
    }
}

void SettingsWindow::setAssociations() {
    for (const u8 idx : range<u8>(0, SUPPORTED_PLAYABLE_EXTENSIONS.size())) {
        if (enabledAssociationsList->item(idx)->checkState() == Qt::Checked) {
            settings->shell.enabledAssociations |= Associations(1 << idx);
        } else {
            settings->shell.enabledAssociations &= ~Associations(1 << idx);
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
    for (const u8 idx : range(1, outputDeviceSelect->count())) {
        if (outputDeviceSelect->itemText(idx) == deviceName) {
            return;
        }
    }

    outputDeviceSelect->addItem(deviceName);
}

void SettingsWindow::onDeviceRemoved(const QString& deviceName) {
    for (const u8 idx : range(1, outputDeviceSelect->count())) {
        if (outputDeviceSelect->itemText(idx) == deviceName) {
            outputDeviceSelect->removeItem(idx);
            break;
        }
    }
}

void SettingsWindow::onOutputDeviceChanged(const u8 index) {
    if (index == 0) {
        settings->core.outputDevice = QString();
        settings->core.outputDeviceID = nullopt;
        return;
    }

    settings->core.outputDevice = playbackDevices[index].name;
    settings->core.outputDeviceID = playbackDevices[index].id;

    emit audioDeviceChanged();
}

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* const ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
};

void SettingsWindow::changeEvent(QEvent* const event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setTrackPropertiesLabels();
        outputDeviceSelect->setItemText(0, tr("Default"));
    }

    QDialog::changeEvent(event);
}

void SettingsWindow::setTrackPropertiesLabels() {
    for (const auto prop : TRACK_PROPERTIES) {
        auto* const item = columnList->item(u8(prop));
        item->setText(
            trackPropertyLabel(TrackProperty(item->data(Qt::UserRole).toUInt()))
        );
        item->setCheckState(
            settings->playlist.columns[u8(prop)].hidden ? Qt::Unchecked
                                                        : Qt::Checked
        );
    }
}
