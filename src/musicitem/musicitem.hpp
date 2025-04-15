#pragma once

#include "aliases.hpp"

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem() { setEditable(false); }

    void setPath(const string& path);
    [[nodiscard]] auto getPath() const -> const string&;

   private:
    string trackPath;
};