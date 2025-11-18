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

    // Core
    for (const QString& style : QStyleFactory::keys()) {
        styleSelect->addItem(style);
    }

    connect(
        themeSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 state) -> void {
        settings->core.applicationTheme = Qt::ColorScheme(state);
        setTheme(Qt::ColorScheme(state));
    }
    );

    if (settings->core.applicationStyle != -1) {
        styleSelect->setCurrentText(
            APPLICATION_STYLES[settings->core.applicationStyle].toString()
        );
    }

    const ma_result devicesFetched = fetchDevices();

    if (devicesFetched != MA_SUCCESS) {
        LOG_ERROR(
            u"Failed to fetch output devices: "_s +
            ma_result_description(devicesFetched)
        );
    }

    settingsDirectoryInput->setText(settings->core.settingsDir);

    const QAction* const locateSettingsDirAction =
        settingsDirectoryInput->addAction(
            QIcon::fromTheme(QIcon::ThemeIcon::EditFind),
            QLineEdit::TrailingPosition
        );

    connect(locateSettingsDirAction, &QAction::triggered, this, [&] -> void {
        const QString dir = QFileDialog::getExistingDirectory(this);

        if (!dir.isEmpty()) {
            settingsDirectoryInput->setText(dir);
        }
    });

    playlistDirectoryInput->setText(settings->core.playlistsDir);

    const QAction* const locatePlaylistDirAction =
        playlistDirectoryInput->addAction(
            QIcon::fromTheme(QIcon::ThemeIcon::EditFind),
            QLineEdit::TrailingPosition
        );

    connect(locatePlaylistDirAction, &QAction::triggered, this, [&] -> void {
        const QString dir = QFileDialog::getExistingDirectory(this);

        if (!dir.isEmpty()) {
            playlistDirectoryInput->setText(dir);
        }
    });

    // Shell
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

    connect(enableAllAudioButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            if (ranges::contains(
                    ALLOWED_AUDIO_EXTENSIONS,
                    enabledAssociationsList->item(idx)->text()
                )) {
                enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
            }
        }
    });

    connect(enableAllVideoButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            if (ranges::contains(
                    ALLOWED_VIDEO_EXTENSIONS,
                    enabledAssociationsList->item(idx)->text()
                )) {
                enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
            }
        }
    });

    connect(enableAllPlaylistsButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            if (ranges::contains(
                    ALLOWED_PLAYLIST_EXTENSIONS,
                    enabledAssociationsList->item(idx)->text()
                )) {
                enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
            }
        }
    });

    connect(enableAllButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Checked);
        }
    });

    connect(clearAllButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            enabledAssociationsList->item(idx)->setCheckState(Qt::Unchecked);
        }
    });

    connect(setButton, &QPushButton::pressed, this, [&] -> void {
        for (const u8 idx : range(0, ALLOWED_PLAYABLE_EXTENSIONS.size())) {
            if (enabledAssociationsList->item(idx)->checkState() ==
                Qt::Checked) {
                settings->shell.enabledAssociations |= Associations(1 << idx);
            } else {
                settings->shell.enabledAssociations &=
                    Associations(~(1 << idx));
            }
        }

        updateFileAssociations(settings->shell.enabledAssociations);
    });

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
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [&](const QString& itemText) -> void {
        const i8 styleIndex = i8(find_index(APPLICATION_STYLES, itemText));
        settings->core.applicationStyle = styleIndex;

        if (styleIndex == -1) {
            QApplication::setStyle(settings->core.defaultStyle);
        } else {
            QApplication::setStyle(itemText);
        }
    }
    );

    connect(
        playlistNamingSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) -> void {
        settings->playlist.playlistNaming = PlaylistNaming(index);
    }
    );

    connect(
        createMenuItemCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        if (checked) {
            createContextMenuEntry();
        } else {
            removeContextMenuEntry();
        }

        settings->shell.contextMenuEntryEnabled = checked;
    }
    );

    connect(
        outputDeviceSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) -> void {
        if (index == outputDeviceSelect->count() - 1) {
            settings->core.outputDevice = QString();
            settings->core.outputDeviceID = std::nullopt;
        } else {
            settings->core.outputDevice = playbackDevices[index].name;
            settings->core.outputDeviceID = playbackDevices[index].id;
        }

        emit audioDeviceChanged();
    }
    );

    connect(
        prioritizeExternalCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        settings->playlist.prioritizeExternalCover = checked;
    }
    );

    connect(
        backgroundImageCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        settings->playlist.autoSetBackground = checked;
    }
    );

    connect(
        settingsList,
        &QListWidget::currentRowChanged,
        this,
        [&](const u16 row) -> void { stackedWidget->setCurrentIndex(row); }
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

auto SettingsWindow::fetchDevices() -> ma_result {
    ma_result result = ma_context_init(nullptr, -1, &config, &context);
    if (result != MA_SUCCESS) {
        return result;
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
        return result;
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

    outputDeviceSelect->addItem(tr("Default Device"));

    if (settings->core.outputDevice.isEmpty()) {
        outputDeviceSelect->setCurrentIndex(outputDeviceSelect->count() - 1);
    };

    return MA_SUCCESS;
}

void SettingsWindow::setTheme(const Qt::ColorScheme colorScheme) {
    QApplication::styleHints()->setColorScheme(colorScheme);
}