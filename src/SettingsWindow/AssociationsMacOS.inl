#pragma once

#include "Enums.hpp"

#include <QString>

// TODO: I have no interest in ever supporting MacOS and figuring out how
// everything works there, so this file will be left unimplemented until some
// contributor that will implement it appears

inline void updateFileAssociationsOS(
    const QString& appPath,
    const QString& iconPath,
    const Associations associations
) {}

inline void createContextMenuEntryOS(const QString& appPath) {}

inline void removeContextMenuEntryOS() {}

[[nodiscard]] inline auto shellEntryExistsOS() -> bool {
    return false;
}

[[nodiscard]] inline auto getExistingAssociationsOS() -> Associations {
    return Associations::None;
}