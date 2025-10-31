#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

inline void createFileAssociationsOS(const QString& appPath, QString iconPath) {
    iconPath.replace(u".ico"_s, u".png"_s);

    const QString desktopEntryPath = u"%1/rap.desktop"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    );
    QFile desktopFile = QFile(desktopEntryPath);

    if (!desktopFile.open(QFile::WriteOnly | QFile::Text)) {
        return;
    }

    QTextStream stream = QTextStream(&desktopFile);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "[Desktop Entry]\n";
    stream << "Name=RAP\n";
    stream << "Exec=" << appPath << " %f\n";
    stream << "Type=Application\n";
    stream << "Icon=" << iconPath << '\n';
    stream << "Categories=AudioVideo;\n";

    for (const QStringView extension : ALLOWED_MUSIC_EXTENSIONS) {
        stream << "MimeType=" << "audio/x-" << extension << ';' << '\n';
    }

    desktopFile.close();

    QProcess::execute(u"update-desktop-database %1"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    ));

    for (const QStringView extension : ALLOWED_MUSIC_EXTENSIONS) {
        QProcess::execute(
            u"xdg-mime default rap.desktop audio/x-%1"_s.arg(extension)
        );
    }
}

inline void removeFileAssociationsOS() {
    const QString desktopEntryPath = u"%1/rap.desktop"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    );

    QFile::remove(desktopEntryPath);

    for (const QStringView extension : ALLOWED_MUSIC_EXTENSIONS) {
        QProcess::execute(
            uR"(xdg-mime default "" audio/x-%1)"_s.arg(extension)
        );
    }

    QProcess::execute(u"update-desktop-database %1"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    ));
}

inline void createContextMenuEntryOS(const QString& appPath) {
    const QString actionsDirPath = u"%1/file-manager/actions"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
    );

    QDir().mkpath(actionsDirPath);

    const QString contextMenuPath =
        u"%1/rap-open-dir.desktop"_s.arg(actionsDirPath);
    QFile contextFile(contextMenuPath);

    if (!contextFile.open(QFile::WriteOnly | QFile::Text)) {
        return;
    }

    QTextStream stream(&contextFile);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "[Desktop Entry]\n";
    stream << "Type=Action\n";
    stream << "Name=Open Folder with RAP\n";
    stream << "Icon=folder\n";
    stream << "Profiles=default;\n\n";

    stream << "[X-Action-Profile default]\n";
    stream << "MimeTypes=inode/directory;\n";
    stream << "Exec=" << appPath << " %f\n";

    contextFile.close();
}

inline void removeContextMenuEntryOS() {
    const QString contextMenuPath =
        u"%1/file-manager/actions/rap-open-dir.desktop"_s.arg(
            QStandardPaths::writableLocation(
                QStandardPaths::GenericDataLocation
            )
        );

    QFile::remove(contextMenuPath);
}