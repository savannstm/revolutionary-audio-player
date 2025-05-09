#include "settingswindow.hpp"

#include <QSettings>
#include <QStyleFactory>
#include <utility>

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
    }
    );
}

SettingsWindow::~SettingsWindow() {
    delete ui;
}

void SettingsWindow::addOpenDirectoryEntry() {
#ifdef Q_OS_WINDOWS
    QSettings directoryEntry(
        uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell\Open directory in RAP)"_s,
        QSettings::NativeFormat
    );

    directoryEntry.setValue("Default", "Open directory in RAP");

    QSettings commandEntry(
        uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell\Open directory in RAP\command)"_s,
        QSettings::NativeFormat
    );

    commandEntry.setValue(
        "Default",
        uR"("%1")"_s.arg(
            QApplication::applicationFilePath().replace('/', '\\')
        ) + uR"( "%1")"_s
    );
#endif
}

void SettingsWindow::removeOpenDirectoryEntry() {
#ifdef Q_OS_WINDOWS
    QSettings directoryEntry(
        uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell\Open directory in RAP)"_s,
        QSettings::NativeFormat
    );
    directoryEntry.setValue("Default", "");

    QSettings commandEntry(
        uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell\Open directory in RAP\command)"_s,
        QSettings::NativeFormat
    );

    commandEntry.setValue("Default", "");

    QSettings topEntry(
        uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\Directory\shell)"_s,
        QSettings::NativeFormat
    );

    topEntry.remove("Open directory in RAP");
#endif
}

void SettingsWindow::associateFileTypes() {
    QString appDir = QApplication::applicationDirPath();
    QString iconPath = appDir + "/icons/rap-logo.ico";
    QString appPath = QApplication::applicationFilePath();

#ifdef Q_OS_WINDOWS

    appDir = appDir.replace('/', '\\');
    iconPath = iconPath.replace('/', '\\');
    appPath = appPath.replace('/', '\\');

    for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
        QSettings extensionEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\rap.%1)"_s.arg(extension),
            QSettings::NativeFormat
        );
        extensionEntry.setValue("Default", extension.toString());

        QSettings defaultIconEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\rap.%1\DefaultIcon)"_s.arg(
                extension
            ),
            QSettings::NativeFormat
        );
        defaultIconEntry.setValue("Default", uR"("%1")"_s.arg(iconPath));

        QSettings shellEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\rap.%1\shell)"_s.arg(
                extension
            ),
            QSettings::NativeFormat
        );
        shellEntry.setValue("Default", "open");

        QSettings openEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\rap.%1\shell\open)"_s.arg(
                extension
            ),
            QSettings::NativeFormat
        );
        openEntry.setValue("Default", "Open in RAP");

        QSettings commandEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\rap.%1\shell\open\command)"_s
                .arg(extension),
            QSettings::NativeFormat
        );

        commandEntry.setValue(
            "Default",
            uR"("%1")"_s.arg(appPath) + uR"( "%1")"_s
        );

        QSettings associationEntry(
            uR"(HKEY_CURRENT_USER\SOFTWARE\Classes\.%1)"_s.arg(extension)
        );

        associationEntry.setValue("Default", u"rap.%1"_s.arg(extension));
    }

#elifdef Q_OS_LINUX

    iconPath = iconPath.replace(".ico", ".png");

    QString desktopEntryPath =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) +
        "/rap.desktop";
    QFile desktopFile(desktopEntryPath);
    if (desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&desktopFile);
        out << "[Desktop Entry]\n";
        out << "Name=RAP\n";
        out << "Exec=" << appPath << " %f\n";
        out << "Type=Application\n";
        out << "Icon=" << iconPath << '\n';
        out << "Categories=AudioVideo;\n";

        QString mimeTypes;

        for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
            const QString mimeType = "audio/x-" + extension.toString();
            mimeTypes += mimeType + ";";
        }

        out << "MimeType=" << mimeTypes << '\n';
        desktopFile.close();
    }

    QProcess::execute(
        "update-desktop-database " +
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    );

    for (const QStringView extension : ALLOWED_FILE_EXTENSIONS) {
        const QString mimeType = "audio/x-" + extension.toString();
        QProcess::execute("xdg-mime default rap.desktop " + mimeType);
    }

#elifdef Q_OS_MACOS
    // TODO
#endif
}