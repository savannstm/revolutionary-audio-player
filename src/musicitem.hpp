#pragma once

#include <QStandardItem>

#include "type_aliases.hpp"

class MusicItem : public QStandardItem {
   public:
    using QStandardItem::QStandardItem;

    void setPath(const path &path) { _path = path; }
    [[nodiscard]] auto getPath() const -> path { return _path; }

   private:
    path _path;
};