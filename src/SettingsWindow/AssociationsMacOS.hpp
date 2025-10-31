#pragma once

#include <QString>

// TODO: I have no interest in ever supporting MacOS and figuring out how
// everything works there, so this file will be left unimplemented until some
// contributor that will implement it appears

inline void
createFileAssociationsOS(const QString& appPath, const QString& iconPath) {}

inline void removeFileAssociationsOS() {}

inline void createContextMenuEntryOS(const QString& appPath_) {}

inline void removeContextMenuEntryOS() {}
