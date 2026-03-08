#include "Associations.hpp"

#include <QApplication>

// TODO: Add support for desktop actions that appear in the context menu of
// taskbar applications (e.g. in KDE Plasma)
#ifdef Q_OS_WINDOWS
#include "AssociationsWindows.inl"

void updateFileAssociations(const Associations associations) {
    QString appDir = qApp->applicationDirPath();
    QString iconPath = appDir + ICO_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                    .toString()
#endif
        ;
    QString appPath = qApp->applicationFilePath();

    updateFileAssociationsOS(
        appPath.replace('/', '\\'),
        iconPath.replace('/', '\\'),
        associations
    );
}

void updateFileAssociationsPath() {
    QString appDir = qApp->applicationDirPath();
    QString iconPath = appDir + ICO_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                    .toString()
#endif
        ;
    QString appPath = qApp->applicationFilePath();

    updateFileAssociationsPathOS(
        appPath.replace('/', '\\'),
        iconPath.replace('/', '\\')
    );
};

void createContextMenuEntry() {
    QString appPath = qApp->applicationFilePath();
    createContextMenuEntryOS(appPath.replace('/', '\\'));
}

void removeContextMenuEntry() {
    removeContextMenuEntryOS();
}

[[nodiscard]] auto getExistingAssociations() -> Associations {
    return getExistingAssociationsOS();
}

[[nodiscard]] auto shellEntryExists() -> bool {
    return shellEntryExistsOS();
}

#elifdef Q_OS_LINUX
#include "AssociationsLinux.inl"

void updateFileAssociations(const Associations associations) {
    const QString appDir = qApp->applicationDirPath();
    const QString iconPath = appDir + PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
        ;
    const QString appPath = qApp->applicationFilePath();

    updateFileAssociationsOS(appPath, iconPath, associations);
}

void updateFileAssociationsPath() {
    const QString appDir = qApp->applicationDirPath();
    const QString iconPath = appDir + PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
        ;
    const QString appPath = qApp->applicationFilePath();

    updateFileAssociationsPathOS(appPath, iconPath);
};

void createContextMenuEntry() {
    const QString appDir = qApp->applicationDirPath();
    const QString iconPath = appDir + PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                          .toString()
#endif
        ;
    const QString appPath = qApp->applicationFilePath();

    createContextMenuEntryOS(appPath, iconPath);
}

void removeContextMenuEntry() {
    removeContextMenuEntryOS();
}

[[nodiscard]] auto getExistingAssociations() -> Associations {
    return getExistingAssociationsOS();
}

[[nodiscard]] auto shellEntryExists() -> bool {
    return shellEntryExistsOS();
}

#elifdef Q_OS_MACOS
#include "AssociationsMacOS.inl"

#else
#error "File associations are not supported for the platform."
#endif
