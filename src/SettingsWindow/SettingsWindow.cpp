#include "SettingsWindow.hpp"

#include "Constants.hpp"
#include "Logger.hpp"

#ifdef Q_OS_WINDOWS
#include "AssociationsWindows.hpp"

#elifdef Q_OS_LINUX
#include "AssociationsLinux.hpp"

#elifdef Q_OS_MACOS
#include "AssociationsMacOS.hpp"
#endif

#include <QStyleFactory>

auto SettingsWindow::setupUi() -> Ui::SettingsWindow* {
    auto* ui_ = new Ui::SettingsWindow();
    ui_->setupUi(this);
    return ui_;
}

SettingsWindow::SettingsWindow(
    shared_ptr<Settings> settings_,
    QWidget* parent
) :
    QDialog(parent),
    settings(std::move(settings_)) {
    for (const QString& style : QStyleFactory::keys()) {
        styleSelect->addItem(style);
    }

    styleSelect->setCurrentText(
        APPLICATION_STYLES[settings->flags.applicationStyle].toString()
    );
    playlistNamingSelect->setCurrentIndex(settings->flags.playlistNaming);
    createMenuItemCheckbox->setChecked(settings->flags.contextMenuEntryEnabled);
    setAssociationsCheckbox->setChecked(
        settings->flags.fileAssociationsEnabled
    );
    backgroundImageCheckbox->setChecked(settings->flags.autoSetBackground);
    prioritizeExternalCheckbox->setChecked(
        settings->flags.prioritizeExternalCover
    );

    ma_result devicesFetched = fetchDevices();

    if (devicesFetched != MA_SUCCESS) {
        LOG_WARN(
            u"Failed to fetch output devices: "_s +
            ma_result_description(devicesFetched)
        );
    }

    connect(
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [&](const QString& itemText) -> void {
        QApplication::setStyle(itemText);

        // Will never be -1
        const u8 styleIndex = find_index(APPLICATION_STYLES, itemText);
        settings->flags.applicationStyle = as<Style>(styleIndex);
    }
    );

    connect(
        playlistNamingSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) -> void {
        settings->flags.playlistNaming = as<PlaylistNaming>(index);
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

        settings->flags.contextMenuEntryEnabled = checked;
    }
    );

    connect(
        setAssociationsCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        if (checked) {
            createFileAssociations();
        } else {
            removeFileAssociations();
        }

        settings->flags.fileAssociationsEnabled = checked;
    }
    );

    connect(
        outputDeviceSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) -> void {
        if (index == outputDeviceSelect->count() - 1) {
            settings->outputDevice = QString();
            settings->outputDeviceID = std::nullopt;
        } else {
            settings->outputDevice = playbackDevices[index].name;
            settings->outputDeviceID = playbackDevices[index].id;
        }

        emit audioDeviceChanged();
    }
    );

    connect(
        prioritizeExternalCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        settings->flags.prioritizeExternalCover = checked;
    }
    );

    connect(
        backgroundImageCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) -> void {
        settings->flags.autoSetBackground = checked;
    }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
    ma_context_uninit(&context);
}

void SettingsWindow::createContextMenuEntry() {
    QString appPath = QApplication::applicationFilePath();
    createContextMenuEntryOS(appPath.replace('/', '\\'));
}

void SettingsWindow::removeContextMenuEntry() {
    removeContextMenuEntryOS();
}

void SettingsWindow::createFileAssociations() {
    QString appDir = QApplication::applicationDirPath();
    QString iconPath = appDir + "/icons/rap-logo.ico";
    QString appPath = QApplication::applicationFilePath();

    createFileAssociationsOS(
        appPath.replace('/', '\\'),
        iconPath.replace('/', '\\')
    );
}

void SettingsWindow::removeFileAssociations() {
    removeFileAssociationsOS();
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

        if (device.name == settings->outputDevice) {
            outputDeviceSelect->setCurrentIndex(
                outputDeviceSelect->count() - 1
            );
        }
    }

    outputDeviceSelect->addItem(tr("Default Device"));

    if (settings->outputDevice.isEmpty()) {
        outputDeviceSelect->setCurrentIndex(outputDeviceSelect->count() - 1);
    };

    return MA_SUCCESS;
}