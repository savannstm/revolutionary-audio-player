#pragma once

#include "aliases.hpp"
#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <QStandardItemModel>

class MusicModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit MusicModel(QObject* parent = nullptr);

    void setItem(u16 row, u16 column, MusicItem* item);

    constexpr void
    setRowMetadata(const u16 row, const metadata_array& metadata) {
        rowMetadata_[row] = metadata;
    };

    void clearRowMetadata() {
        rowMetadata_.clear();
        tracks.clear();
    };

    [[nodiscard]] auto rowMetadata(const u16 row) const
        -> const metadata_array& {
        return rowMetadata_.find(row)->second;
    };

    [[nodiscard]] auto rowProperty(
        const u16 row,
        const TrackProperty property
    ) const -> const QString& {
        return rowMetadata_.find(row)->second[property];
    };

    void removeRowMetadata(const u16 row) { rowMetadata_.erase(row); };

    [[nodiscard]] auto contains(const QString& path) const -> bool {
        return tracks.contains(&path);
    };

    void sort(i32 column, Qt::SortOrder order = Qt::AscendingOrder) override;

    auto itemFromIndex(const QModelIndex& index) -> MusicItem*;

   private:
    rapidhashset<const QString*> tracks;
    rapidhashmap<u16, metadata_array> rowMetadata_;
};
