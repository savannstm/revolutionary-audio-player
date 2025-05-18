#pragma once

#include "aliases.hpp"
#include "constants.hpp"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

void createFileAssociationsLinux(const QString& appPath, QString iconPath) {
    iconPath = iconPath.replace(".ico", ".png");

    const QString desktopEntryPath =
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

        for (const QStringView extension : ALLOWED_MUSIC_FILE_EXTENSIONS) {
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

    for (const QStringView extension : ALLOWED_MUSIC_FILE_EXTENSIONS) {
        const QString mimeType = u"audio/x-%1"_s.arg(extension);
        QProcess::execute("xdg-mime default rap.desktop " + mimeType);
    }
}