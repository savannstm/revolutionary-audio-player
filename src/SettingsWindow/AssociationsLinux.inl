#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

enum class DesktopEnvironment : u8 {
    KDE,
    XFCE,
    GNOME,
    Cinnamon,
    Unknown,
};

void updateMimeappsList(const QStringList& mimeTypes) {
    const QString configPath =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
        u"/mimeapps.list";

    QDir().mkpath(QFileInfo(configPath).absolutePath());

    QFile configFile = QFile(configPath);
    if (!configFile.open(QFile::ReadWrite)) {
        qCritical() << "Failed to open config file"_L1;
        return;
    }

    QTextStream stream = QTextStream(&configFile);
    stream.setEncoding(QStringConverter::Utf8);

    QMap<QString, QStringList> sections;
    QString currentSection;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();

        const QString trimmed = line.trimmed();
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            currentSection = trimmed.mid(1, trimmed.size() - 2);
        } else {
            sections[currentSection].append(line);
        }
    }

    QMap<QString, QStringList> defaults;

    const QStringList defaultLines = sections.value(u"Default Applications"_s);
    for (const QString& line : defaultLines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }

        const qsizetype eqPos = trimmed.indexOf('=');
        if (eqPos <= 0) {
            continue;
        }

        const QString mime = trimmed.left(eqPos);
        const QString appsStr = trimmed.mid(eqPos + 1);

        QStringList apps = appsStr.split(';', Qt::SkipEmptyParts);

        apps.removeAll(u"rap.desktop");

        if (!apps.isEmpty()) {
            defaults[mime] = apps;
        }
    }

    for (const QString& mime : mimeTypes) {
        defaults[mime].removeAll(u"rap.desktop");
        defaults[mime].prepend(u"rap.desktop"_s);
    }

    QStringList newDefaults;
    newDefaults.reserve(defaults.size());

    for (const auto& [key, value] : defaults.asKeyValueRange()) {
        newDefaults.append(key + '=' + value.join(';') + ';');
    }

    sections[u"Default Applications"_s] = newDefaults;

    configFile.resize(0);
    stream.seek(0);

    for (const auto& [key, value] : sections.asKeyValueRange()) {
        if (!key.isEmpty()) {
            stream << '[' << key << "]\n";
        }

        for (const QString& line : value) {
            stream << line << '\n';
        }

        stream << '\n';
    }
}

inline void updateFileAssociationsOS(
    const QString& appPath,
    const QString& iconPath,
    const Associations associations
) {
    const QString appsLocation =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopFilePath = appsLocation + u"/rap.desktop"_qssv;

    auto desktopFile = QFile(desktopFilePath);

    if (associations == Associations::None) {
        updateMimeappsList({});
        QFile::remove(desktopFilePath);
        QProcess::execute(u"update-desktop-database"_s, { appsLocation });
        return;
    }

    if (!desktopFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << (desktopFile.errorString());
        return;
    }

    auto stream = QTextStream(&desktopFile);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "[Desktop Entry]\n";
    stream << "Name=RAP\n";
    stream << "Type=Application\n";
    stream << "Categories=Qt;AudioVideo;Player;\n";
    stream << "Exec=" << appPath << " %U\n";
    stream << "Icon=" << iconPath << '\n';
    stream << "MimeType=";

    QStringList mimeTypes;
    mimeTypes.reserve(u16(SUPPORTED_EXTENSIONS_COUNT * 4));

    bool mpegAdded = false;

    for (const auto [idx, ext] :
         views::enumerate(SUPPORTED_PLAYABLE_EXTENSIONS)) {
        const auto association = Associations(1 << idx);

        if ((associations & association) != Associations::None) {
            if (idx < (SUPPORTED_AUDIO_EXTENSIONS_COUNT) +
                          (SUPPORTED_VIDEO_EXTENSIONS_COUNT)) {
                mimeTypes.append(
                    u"audio/x-"_s + ext
#if QT_VERSION_MINOR < 9
                                        .toString()
#endif
                );
                mimeTypes.append(
                    u"audio/"_s + ext
#if QT_VERSION_MINOR < 9
                                      .toString()
#endif
                );
                mimeTypes.append(
                    u"video/x-"_s + ext
#if QT_VERSION_MINOR < 9
                                        .toString()
#endif
                );
                mimeTypes.append(
                    u"video/"_s + ext
#if QT_VERSION_MINOR < 9
                                      .toString()
#endif
                );

                if (((association & Associations::MP3) != Associations::None ||
                     (association & Associations::M4A) != Associations::None ||
                     (association & Associations::MP4) != Associations::None) &&
                    !mpegAdded) {
                    mimeTypes.append(u"audio/mpeg"_s);
                    mimeTypes.append(u"video/mpeg"_s);
                    mimeTypes.append(u"audio/x-mpeg"_s);
                    mimeTypes.append(u"video/x-mpeg"_s);
                    mpegAdded = true;
                }
            } else {
                if ((association & Associations::M3U) != Associations::None ||
                    (association & Associations::M3U8) != Associations::None) {
                    mimeTypes.append(u"audio/x-mpegurl"_s);
                    mimeTypes.append(u"application/vnd.apple.mpegurl"_s);
                    mimeTypes.append(u"video/vnd.mpegurl"_s);
                } else if ((association & Associations::XSPF) !=
                           Associations::None) {
                    mimeTypes.append(u"application/xspf+xml"_s);
                } else if ((association & Associations::CUE) !=
                           Associations::None) {
                    mimeTypes.append(u"application/x-cue"_s);
                    mimeTypes.append(u"text/x-cue"_s);
                }
            }
        }
    }

    stream << mimeTypes.join(';') << '\n';

    updateMimeappsList(mimeTypes);
    QProcess::execute(u"update-desktop-database"_s, { appsLocation });
}

inline void
updateFileAssociationsPathOS(const QString& appPath, const QString& iconPath) {
    const QString appsLocation =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopEntryPath = appsLocation + u"/rap.desktop"_qssv;

    QFile desktopFile = QFile(desktopEntryPath);
    if (!desktopFile.open(QFile::ReadWrite | QFile::Text)) {
        qWarning() << (desktopFile.errorString());
        return;
    }

    QTextStream stream = QTextStream(&desktopFile);
    stream.setEncoding(QStringConverter::Utf8);

    QStringList lines;
    lines.reserve(7);

    const QString newExecLine = u"Exec="_s + appPath + u" %U";
    const QString newIconLine = u"Icon="_s + iconPath;

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (line.startsWith(u"Exec=")) {
            line = newExecLine;
        } else if (line.startsWith(u"Icon=")) {
            line = newIconLine;
        }

        lines.append(line);
    }

    desktopFile.resize(0);
    stream.seek(0);

    for (const QString& line : lines) {
        stream << line << '\n';
    }

    QProcess::execute(u"update-desktop-database"_s, { appsLocation });
}

inline auto detectDesktopEnvironment() -> DesktopEnvironment {
    QString desktopEnvironment =
        qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();

    if (desktopEnvironment.isEmpty()) {
        desktopEnvironment = qEnvironmentVariable("DESKTOP_SESSION").toLower();
    }

    if (desktopEnvironment.contains(u"xfce")) {
        return DesktopEnvironment::XFCE;
    }

    if (desktopEnvironment.contains(u"gnome")) {
        return DesktopEnvironment::GNOME;
    }

    if (desktopEnvironment.contains(u"cinnamon")) {
        return DesktopEnvironment::Cinnamon;
    }

    if (desktopEnvironment.contains(u"kde") ||
        desktopEnvironment.contains(u"plasma")) {
        return DesktopEnvironment::KDE;
    }

    return DesktopEnvironment::Unknown;
}

inline void
createContextMenuEntryOS(const QString& appPath, const QString& iconPath) {
    const DesktopEnvironment desktopEnvironment = detectDesktopEnvironment();
    const QString dataLocation =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QFlags<QFile::OpenModeFlag> openFlags =
        QFile::WriteOnly | QFile::Text | QFile::Truncate;

    switch (desktopEnvironment) {
        case DesktopEnvironment::KDE: {
            const QString servicesDirPath =
                dataLocation + u"/kio/servicemenus"_qssv;
            QDir().mkpath(servicesDirPath);

            const QString serviceMenuPath =
                servicesDirPath + u"/open-dir-in-rap.desktop"_qssv;

            auto serviceFile = QFile(serviceMenuPath);

            if (!serviceFile.open(openFlags)) {
                qWarning() << "Failed to open service file: "_L1
                           << serviceFile.errorString();
                return;
            }

            QFile::setPermissions(
                serviceMenuPath,
                QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                    QFile::ReadGroup | QFile::ReadOther
            );

            auto stream = QTextStream(&serviceFile);
            stream.setEncoding(QStringConverter::Utf8);

            stream << "[Desktop Entry]\n";
            stream << "Type=Service\n";
            stream << "X-KDE-ServiceTypes=KonqPopupMenu/Plugin\n";
            stream << "MimeType=inode/directory;\n";
            stream << "Actions=openinrap;\n";
            stream << '\n';
            stream << "[Desktop Action openinrap]\n";
            stream << "Name=Open Directory in RAP\n";
            stream << "Exec=" << appPath << " %U\n";
            stream << "Icon=" << iconPath << '\n';

            QProcess::execute(u"kbuildsycoca6"_s);
            break;
        }
        case DesktopEnvironment::XFCE: {
            const QString actionsDirPath =
                dataLocation + u"/file-manager/actions"_qssv;
            QDir().mkpath(actionsDirPath);

            const QString contextMenuPath =
                actionsDirPath + u"/open-dir-in-rap.desktop"_qssv;
            auto contextFile = QFile(contextMenuPath);

            if (!contextFile.open(openFlags)) {
                qWarning()
                    << (u"Failed to open context menu file: "_s +
                        contextFile.errorString());
                return;
            }

            auto stream = QTextStream(&contextFile);
            stream.setEncoding(QStringConverter::Utf8);

            stream << "[Desktop Entry]\n";
            stream << "Type=Action\n";
            stream << "Name=Open Directory in RAP\n";
            stream << "Icon=" << iconPath << '\n';
            stream << "[X-Action-Profile default]\n";
            stream << "MimeType=inode/directory;\n";
            stream << "Exec=" << appPath << " %U\n";
            contextFile.close();

            QProcess::execute(u"update-desktop-database"_s, { actionsDirPath });
            break;
        }
        case DesktopEnvironment::GNOME: {
            const QString extensionsDir =
                dataLocation + u"/nautilus-python/extensions"_qssv;
            QDir().mkpath(extensionsDir);

            const QString extensionPath =
                extensionsDir + u"/open_dir_in_rap.py"_qssv;

            QFile extensionFile = QFile(extensionPath);

            if (!extensionFile.open(openFlags)) {
                qWarning()
                    << (u"Failed to open Nautilus extension file: "_s +
                        extensionFile.errorString());
                return;
            }

            QTextStream stream = QTextStream(&extensionFile);
            stream.setEncoding(QStringConverter::Utf8);

            stream <<
                R"(from gi.repository import Nautilus, GObject
import subprocess
import shlex

class OpenDirInRAP(GObject.GObject, Nautilus.MenuProvider):
    def get_file_items(self, window, files):
        if len(files) != 1:
            return []

        f = files[0]
        if not f.is_directory():
            return []

        item = Nautilus.MenuItem(
            name="OpenDirInRAP::open",
            label="Open Directory in RAP",
            tip="Open this directory in RAP"
        )

        def activate(menu, file=f):
            uri = file.get_location().get_path()
            subprocess.Popen([)"
                   << '"' << appPath << '"' << R"(, uri])

        item.connect("activate", activate)
        return [item]
)";

            extensionFile.close();

            QFile::setPermissions(
                extensionPath,
                QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                    QFile::ReadGroup | QFile::ReadOther
            );

            QProcess::execute(u"nautilus"_s, { u"-q"_s });

            break;
        }
        case DesktopEnvironment::Cinnamon: {
            const QString actionsDirPath = dataLocation + u"/nemo/actions"_qssv;
            QDir().mkpath(actionsDirPath);

            const QString contextMenuPath =
                actionsDirPath + u"/open-dir-in-rap.nemo_action"_qssv;
            auto contextFile = QFile(contextMenuPath);

            if (!contextFile.open(openFlags)) {
                qWarning()
                    << (u"Failed to open Nemo action file: "_s +
                        contextFile.errorString());
                return;
            }

            auto stream = QTextStream(&contextFile);
            stream.setEncoding(QStringConverter::Utf8);

            stream << "[Nemo Action]\n";
            stream << "Name=Open Directory in RAP\n";
            stream << "Comment=Open this directory in RAP\n";
            stream << "Exec=" << appPath << " %U\n";
            stream << "Icon-Name=" << iconPath << '\n';
            stream << "Selection=any\n";
            stream << "Extensions=dir;\n";
            break;
        }
        case DesktopEnvironment::Unknown:
            qWarning()
                << "Unsupported or unknown desktop environment. No context menu created."_L1;
            return;
    }
}

inline void removeContextMenuEntryOS() {
    const DesktopEnvironment desktopEnvironment = detectDesktopEnvironment();
    const QString dataLocation =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    QString path;

    switch (desktopEnvironment) {
        case DesktopEnvironment::KDE:
            path = dataLocation +
                   u"/kio/servicemenus/open-dir-in-rap.desktop"_qssv;
            break;
        case DesktopEnvironment::XFCE:
            path = dataLocation +
                   u"/file-manager/actions/open-dir-in-rap.desktop"_qssv;
            break;
        case DesktopEnvironment::GNOME:
            path = dataLocation +
                   u"/nautilus-python/extensions/open_dir_in_rap.py"_qssv;
            break;
        case DesktopEnvironment::Cinnamon:
            path = dataLocation +
                   u"/nemo/actions/open-dir-in-rap.nemo_action"_qssv;
            break;
        case DesktopEnvironment::Unknown:
            return;
    }

    if (QFile::exists(path)) {
        QFile::remove(path);

        if (desktopEnvironment == DesktopEnvironment::KDE) {
            QProcess::execute(u"kbuildsycoca6"_s);
        }

        if (desktopEnvironment == DesktopEnvironment::GNOME) {
            QProcess::execute(u"nautilus"_s, { u"-q"_s });
        }
    }
}

[[nodiscard]] inline auto shellEntryExistsOS() -> bool {
    const DesktopEnvironment desktopEnvironment = detectDesktopEnvironment();
    const QString dataLocation =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    QString path;

    switch (desktopEnvironment) {
        case DesktopEnvironment::KDE:
            path = dataLocation +
                   u"/kio/servicemenus/open-dir-in-rap.desktop"_qssv;
            break;
        case DesktopEnvironment::XFCE:
            path = dataLocation +
                   u"/file-manager/actions/open-dir-in-rap.desktop"_qssv;
            break;
        case DesktopEnvironment::GNOME:
            path = dataLocation +
                   u"/nautilus-python/extensions/open_dir_in_rap.py"_qssv;
            break;
        case DesktopEnvironment::Cinnamon:
            path = dataLocation +
                   u"/nemo/actions/open-dir-in-rap.nemo_action"_qssv;
            break;
        case DesktopEnvironment::Unknown:
            return false;
    }

    return QFile::exists(path);
}

[[nodiscard]] inline auto getExistingAssociationsOS() -> Associations {
    const QString appsLocation =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopEntryPath = appsLocation + u"/rap.desktop"_qssv;

    Associations associations = Associations::None;

    if (QFile::exists(desktopEntryPath)) {
        auto desktopFile = QFile(desktopEntryPath);

        if (!desktopFile.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << (desktopFile.errorString());
            return associations;
        }

        while (!desktopFile.atEnd()) {
            QString line = desktopFile.readLine();

            if (line.startsWith(u"MimeType=")) {
                for (const auto [idx, ext] :
                     views::enumerate(SUPPORTED_PLAYABLE_EXTENSIONS)) {
                    const bool isCue =
                        (ext == EXT_CUE && line.contains(u"text/x-cue"));

                    const bool isXSPF =
                        (ext == EXT_XSPF &&
                         line.contains(u"application/xspf+xml"));

                    const bool isM3U =
                        ((ext == EXT_M3U || ext == EXT_M3U8) &&
                         line.contains(u"audio/x-mpegurl"));

                    const bool isAudioVideo = line.contains(
                        u"audio/"_s + ext
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
                    );

                    if (isCue || isXSPF || isM3U || isAudioVideo) {
                        associations |= Associations(1 << idx);
                    }
                }

                break;
            }
        }
    }

    return associations;
}