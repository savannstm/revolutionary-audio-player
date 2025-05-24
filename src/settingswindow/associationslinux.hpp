#pragma once

#include "aliases.hpp"
#include "constants.hpp"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

void createFileAssociationsOS(const QString& appPath, QString iconPath) {
    iconPath.replace(u".ico"_s, u".png"_s);

    const QString desktopEntryPath = u"%1/rap.desktop"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    );
    QFile desktopFile(desktopEntryPath);

    if (!desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&desktopFile);
    out << "[Desktop Entry]\n";
    out << "Name=RAP\n";
    out << "Exec=" << appPath << " %f\n";
    out << "Type=Application\n";
    out << "Icon=" << iconPath << '\n';
    out << "Categories=AudioVideo;\n";

    for (const QStringView extension : ALLOWED_MUSIC_FILE_EXTENSIONS) {
        out << "MimeType=" << "audio/x-" << extension << ';' << '\n';
    }

    desktopFile.close();

    QProcess::execute(u"update-desktop-database %1"_s.arg(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
    ));

    for (const QStringView extension : ALLOWED_MUSIC_FILE_EXTENSIONS) {
        QProcess::execute(
            u"xdg-mime default rap.desktop audio/x-%1"_s.arg(extension)
        );
    }
}

void createContextMenuEntryOS(const QString& appPath_) {}

void removeContextMenuEntryOS() {}

void removeFileAssociationsOS() {}