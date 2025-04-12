#pragma once

#include "aliases.hpp"

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    using QStandardItem::QStandardItem;

    constexpr void setPath(const path& newPath) { path = newPath; }

    [[nodiscard]] constexpr auto getPath() const -> path { return path; }

   private:
    path path;
};