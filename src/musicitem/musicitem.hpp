#pragma once

#include "aliases.hpp"

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem();
    explicit MusicItem(const QString& text);

    void setPath(const string& path);
    [[nodiscard]] auto getPath() const -> const string&;

   private:
    string trackPath;
};