#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

enum class DesktopEnvironment : u8 {
    KDE,
    XFCE,
    GNOME,
    Cinnamon,
    Unknown,
};

inline void updateFileAssociationsOS(
    const QString& appPath,
    const QString& iconPath,
    const Associations associations
) {
    const QString appsLocation =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QString desktopEntryPath = appsLocation + u"/rap.desktop"_qssv;

    auto desktopFile = QFile(desktopEntryPath);

    if (associations == Associations::None) {
        QFile::remove(desktopEntryPath);
        QProcess::execute(u"update-desktop-database"_s, { appsLocation });
        return;
    }

    // Build the .desktop file content
    if (!desktopFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        LOG_WARN(desktopFile.errorString());
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
    mimeTypes.reserve(u16(ALLOWED_EXTENSIONS_COUNT * 4));

    bool mpegAdded = false;

    for (const auto [idx, ext] :
         views::enumerate(ALLOWED_PLAYABLE_EXTENSIONS)) {
        const auto association = Associations(1 << idx);

        if (associations & association) {
            if (idx < ALLOWED_AUDIO_EXTENSIONS_COUNT +
                          ALLOWED_VIDEO_EXTENSIONS_COUNT) {
                mimeTypes << u"audio/x-"_s + ext
#if QT_VERSION_MINOR < 9
                                                 .toString()
#endif
                    ;
                mimeTypes << u"audio/"_s + ext
#if QT_VERSION_MINOR < 9
                                               .toString()
#endif
                    ;
                mimeTypes << u"video/x-"_s + ext
#if QT_VERSION_MINOR < 9
                                                 .toString()
#endif
                    ;
                mimeTypes << u"video/"_s + ext
#if QT_VERSION_MINOR < 9
                                               .toString()
#endif
                    ;

                if ((association & Associations::MP3 ||
                     association & Associations::M4A ||
                     association & Associations::MP4) &&
                    !mpegAdded) {
                    mimeTypes << u"audio/mpeg"_s;
                    mimeTypes << u"video/mpeg"_s;
                    mimeTypes << u"audio/x-mpeg"_s;
                    mimeTypes << u"video/x-mpeg"_s;
                }
            } else if (idx < ALLOWED_AUDIO_EXTENSIONS_COUNT +
                                 ALLOWED_VIDEO_EXTENSIONS_COUNT +
                                 ALLOWED_PLAYLIST_EXTENSIONS_COUNT) {
                if (association & Associations::M3U ||
                    association & Associations::M3U8) {
                    mimeTypes << u"audio/x-mpegurl"_s;
                    mimeTypes << u"application/vnd.apple.mpegurl"_s;
                    mimeTypes << u"video/vnd.mpegurl"_s;
                } else if (association & Associations::XSPF) {
                    mimeTypes << u"application/xspf+xml"_s;
                } else if (association & Associations::CUE) {
                    mimeTypes << u"application/x-cue"_s;
                    mimeTypes << u"text/x-cue"_s;
                }
            }
        }
    }

    stream << mimeTypes.join(';') << '\n';

    desktopFile.close();

    for (const QString& mimeType : mimeTypes) {
        QProcess::execute(
            u"xdg-mime"_s,
            { u"default"_s, u"rap.desktop"_s, mimeType }
        );
    }

    QProcess::execute(u"update-desktop-database"_s, { appsLocation });
}

inline auto detectDesktopEnvironment() -> DesktopEnvironment {
    QString desktopEnvironment =
        qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();

    if (desktopEnvironment.isEmpty()) {
        desktopEnvironment = qEnvironmentVariable("DESKTOP_SESSION").toLower();
    }

    if (desktopEnvironment.contains(u"xfce"_qsv)) {
        return DesktopEnvironment::XFCE;
    }

    if (desktopEnvironment.contains(u"gnome"_qsv)) {
        return DesktopEnvironment::GNOME;
    }

    if (desktopEnvironment.contains(u"cinnamon"_qsv)) {
        return DesktopEnvironment::Cinnamon;
    }

    if (desktopEnvironment.contains(u"kde"_qsv) ||
        desktopEnvironment.contains(u"plasma"_qsv)) {
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

            QFile::setPermissions(serviceMenuPath, QFile::ExeOwner);
            auto serviceFile = QFile(serviceMenuPath);

            // FIXME: Not enough privileges?
            if (!serviceFile.open(openFlags)) {
                LOG_WARN(
                    u"Failed to open service file: "_s +
                    serviceFile.errorString()
                );
                return;
            }

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
                LOG_WARN(
                    u"Failed to open context menu file: "_s +
                    contextFile.errorString()
                );
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
        case DesktopEnvironment::GNOME:
            // TODO
            break;
        case DesktopEnvironment::Cinnamon: {
            const QString actionsDirPath = dataLocation + u"/nemo/actions"_qssv;
            QDir().mkpath(actionsDirPath);

            const QString contextMenuPath =
                actionsDirPath + u"/open-dir-in-rap.nemo_action"_qssv;
            auto contextFile = QFile(contextMenuPath);

            if (!contextFile.open(openFlags)) {
                LOG_WARN(
                    u"Failed to open Nemo action file: "_s +
                    contextFile.errorString()
                );
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
            LOG_WARN(
                u"Unsupported or unknown desktop environment. No context menu created."_s
            );
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
            // TODO
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
            // TODO
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
            LOG_WARN(desktopFile.errorString());
            return associations;
        }

        while (!desktopFile.atEnd()) {
            QString line = desktopFile.readLine();

            if (line.startsWith(u"MimeType="_qsv)) {
                const isize start = sizeof("MimeType=") - 1;
                const isize end = line.lastIndexOf(';');

                const QString& extensions = line.slice(start, end - start);

                for (const auto [idx, ext] :
                     views::enumerate(ALLOWED_PLAYABLE_EXTENSIONS)) {
                    if ((ext == EXT_CUE &&
                         extensions.contains(u"text/x-cue"_qsv)) ||
                        (ext == EXT_XSPF &&
                         extensions.contains(u"application/xspf+xml"_qsv)) ||
                        ((ext == EXT_M3U || ext == EXT_M3U8) &&
                         extensions.contains(u"audio/x-mpegurl"_qsv)) ||
                        extensions.contains(
                            u"audio/"_s + ext
#if QT_VERSION_MINOR < 9
                                              .toString()
#endif
                        )) {
                        associations &= Associations(1 << idx);
                    }
                }

                break;
            }
        }
    }

    return associations;
}