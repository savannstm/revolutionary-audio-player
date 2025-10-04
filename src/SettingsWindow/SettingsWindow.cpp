#include "SettingsWindow.hpp"

#include "Constants.hpp"

#ifdef Q_OS_WINDOWS
#include "AssociationsWindows.hpp"

#elifdef Q_OS_LINUX
#include "AssociationsLinux.hpp"

#elifdef Q_OS_MACOS
#include "AssociationsMacOS.hpp"
#endif

#include <QAudioDevice>
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

    connect(
        styleSelect,
        &QComboBox::currentTextChanged,
        this,
        [&](const QString& itemText) {
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

    connect(
        prioritizeExternalCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) {
        settings->flags.prioritizeExternalCover = checked;
    }
    );

    connect(
        backgroundImageCheckbox,
        &QCheckBox::toggled,
        this,
        [&](const bool checked) { settings->flags.autoSetBackground = checked; }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
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