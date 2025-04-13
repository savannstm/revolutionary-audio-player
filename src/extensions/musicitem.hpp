#pragma once

#include "aliases.hpp"

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    using QStandardItem::QStandardItem;

    constexpr void setPath(const path& newPath) { _path = newPath; };

    [[nodiscard]] constexpr auto getPath() const -> path { return _path; };

   private:
    path _path;
};