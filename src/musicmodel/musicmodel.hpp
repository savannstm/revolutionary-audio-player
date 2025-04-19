#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <QStandardItemModel>

class MusicModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit MusicModel(QObject* parent = nullptr);

    void setItem(u16 row, u16 column, MusicItem* item);
    void setRowMetadata(u16 row, const metadata_array& metadata);
    void clearRowMetadata();

    [[nodiscard]] auto getRowMetadata(u16 row) const -> const metadata_array&;
    [[nodiscard]] auto getRowPath(u16 row) const -> const string&;
    [[nodiscard]] auto contains(const string& path) const -> bool;

    void sort(i32 column, Qt::SortOrder order = Qt::AscendingOrder) override;

   private:
    rapidhashset<string> tracks;
    rapidhashmap<u32, metadata_array> rowMetadata;
};
