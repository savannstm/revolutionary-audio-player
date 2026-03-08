#pragma once

#include "Enums.hpp"

void updateFileAssociations(Associations associations);
void updateFileAssociationsPath();
void createContextMenuEntry();
void removeContextMenuEntry();

[[nodiscard]] auto getExistingAssociations() -> Associations;
[[nodiscard]] auto shellEntryExists() -> bool;
