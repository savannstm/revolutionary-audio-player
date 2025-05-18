#include "settingswindow.hpp"

#include "associationswindows.hpp"

#include <QAudioDevice>
#include <QSettings>
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
    dragDropSelect->setCurrentIndex(settings->flags.dragNDropMode);
    playlistNamingSelect->setCurrentIndex(settings->flags.playlistNaming);
    createMenuItemCheckbox->setChecked(settings->flags.contextMenuEntryEnabled);
    setAssociationsCheckbox->setChecked(
        settings->flags.fileAssociationsEnabled
    );

    connect(
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [&](const QString& itemText) {
        QApplication::setStyle(itemText);

        for (const auto [idx, style] : views::enumerate(APPLICATION_STYLES)) {
            if (itemText == style) {
                settings->flags.applicationStyle = as<Style>(idx);
                break;
            }
        }
    }
    );

    connect(
        dragDropSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) {
        settings->flags.dragNDropMode = as<DragDropMode>(index);
    }
    );

    connect(
        playlistNamingSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) {
        settings->flags.playlistNaming = as<PlaylistNaming>(index);
    }
    );

    connect(
        createMenuItemCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) {
        if (checked) {
            addOpenDirectoryEntry();
        } else {
            removeOpenDirectoryEntry();
        }

        settings->flags.contextMenuEntryEnabled = checked;
    }
    );

    connect(
        setAssociationsCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) {
        if (checked) {
            createFileAssociations();
        } else {
            removeFileAssociations();
        }

        settings->flags.fileAssociationsEnabled = checked;
    }
    );

    for (const QAudioDevice& device : devices) {
        const QString description = device.description();
        outputDeviceSelect->addItem(description);

        if (description == settings->outputDevice.description()) {
            outputDeviceSelect->setCurrentIndex(
                outputDeviceSelect->count() - 1
            );
        }
    }

    connect(
        outputDeviceSelect,
        &QComboBox::currentIndexChanged,
        this,
        [&](const u8 index) {
        settings->outputDevice = devices[index];
        emit audioDeviceChanged(devices[index]);
    }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}

void SettingsWindow::addOpenDirectoryEntry() {
    QString appPath = QApplication::applicationFilePath();

#ifdef Q_OS_WINDOWS
    createContextMenuDirectoryEntryWindows(wstring(
        ras<const wchar*>(appPath.replace('/', '\\').utf16()),
        appPath.length()
    ));

#elifdef Q_OS_LINUX
    // TODO
#elifdef Q_OS_MACOS
    // TODO
#endif
}

void SettingsWindow::removeOpenDirectoryEntry() {
#ifdef Q_OS_WINDOWS
    removeContextMenuDirectoryEntryWindows();

#elifdef Q_OS_LINUX
    // TODO
#elifdef Q_OS_MACOS
    // TODO
#endif
}

void SettingsWindow::createFileAssociations() {
    QString appDir = QApplication::applicationDirPath();
    QString iconPath = appDir + "/icons/rap-logo.ico";
    QString appPath = QApplication::applicationFilePath();

#ifdef Q_OS_WINDOWS
    appDir = appDir.replace('/', '\\');
    iconPath = iconPath.replace('/', '\\');
    appPath = appPath.replace('/', '\\');

    createFileAssociationsWindows(
        wstring(ras<const wchar*>(appPath.utf16()), appPath.length()),
        wstring(ras<const wchar*>(iconPath.utf16()), iconPath.length())
    );
#elifdef Q_OS_LINUX
    // TODO
#elifdef Q_OS_MACOS
    // TODO
#endif
}

void SettingsWindow::removeFileAssociations() {
#ifdef Q_OS_WINDOWS
    removeFileAssociationsWindows();

#elifdef Q_OS_LINUX
    // TODO
#elifdef Q_OS_MACOS
    // TODO
#endif
}