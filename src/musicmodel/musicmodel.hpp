#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "musicitem.hpp"

#include <QStandardItemModel>

struct RowInfo {
    u32 originalIndex;
    QList<QStandardItem*> items;
};

class MusicModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit MusicModel(QObject* parent = nullptr);

    void setItem(u16 row, u16 column, MusicItem* item);
    void setRowMetadata(u16 row, const array<string, PROPERTY_COUNT>& metadata);
    [[nodiscard]] auto getRowMetadata(u16 row) const
        -> const array<string, PROPERTY_COUNT>&;
    [[nodiscard]] auto getRowPath(u16 row) const -> const string&;
    [[nodiscard]] auto contains(const path& path) const -> bool;

    void sort(i32 column, Qt::SortOrder order = Qt::AscendingOrder) override;

   private:
    hashset<path> tracks;
    hashmap<u32, array<string, PROPERTY_COUNT>> rowMetadata;
};
