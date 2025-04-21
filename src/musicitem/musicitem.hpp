#pragma once

#include <QStandardItem>

class MusicItem : public QStandardItem {
   public:
    explicit MusicItem();
    explicit MusicItem(const QString& text);

    void setPath(const QString& path);
    [[nodiscard]] auto getPath() const -> const QString&;

   private:
    QString trackPath;
};