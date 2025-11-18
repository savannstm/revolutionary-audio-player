#include "Associations.hpp"

#include <QApplication>

#ifdef Q_OS_WINDOWS
#include "AssociationsWindows.inl"

void updateFileAssociations(const Associations associations) {
    QString appDir = QApplication::applicationDirPath();
    QString iconPath = appDir + '/' +
                       ICO_LOGO_PATH
#if QT_VERSION_MINOR < 9
                           .toString()
#endif
        ;
    QString appPath = QApplication::applicationFilePath();

    updateFileAssociationsOS(
        appPath.replace('/', '\\'),
        iconPath.replace('/', '\\'),
        associations
    );
}

void createContextMenuEntry() {
    QString appPath = QApplication::applicationFilePath();
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
    const QString appDir = QApplication::applicationDirPath();
    const QString iconPath = appDir + '/' +
                             PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                 .toString()
#endif
        ;
    const QString appPath = QApplication::applicationFilePath();

    updateFileAssociationsOS(appPath, iconPath, associations);
}

void createContextMenuEntry() {
    const QString appDir = QApplication::applicationDirPath();
    const QString iconPath = appDir + '/' +
                             PNG_LOGO_PATH
#if QT_VERSION_MINOR < 9
                                 .toString()
#endif
        ;
    const QString appPath = QApplication::applicationFilePath();

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
