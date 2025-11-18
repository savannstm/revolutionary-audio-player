#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "MusicItem.hpp"
#include "RapidHasher.hpp"

#include <QStandardItemModel>

class TrackTreeModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit TrackTreeModel(QObject* parent = nullptr);

    [[nodiscard]] auto item(const i32 row, const i32 column = 0) const
        -> MusicItem* {
        return as<MusicItem*>(QStandardItemModel::item(row, column));
    }

    [[nodiscard]] auto itemFromIndex(const QModelIndex& index) const
        -> MusicItem*;

    [[nodiscard]] auto trackProperty(u8 column) const -> TrackProperty;
    auto
    removeRows(i32 row, i32 count, const QModelIndex& parent = QModelIndex())
        -> bool override;

    void setItem(u16 row, u16 column, QStandardItem* item, bool path = false);

    [[nodiscard]] auto contains(const QString& path) const -> bool {
        return tracks.contains(path);
    };

   protected:
    [[nodiscard]] auto flags(const QModelIndex& index) const
        -> Qt::ItemFlags override {
        const Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

        if (!index.isValid()) {
            return Qt::ItemIsDropEnabled | defaultFlags;
        }

        return defaultFlags & ~Qt::ItemIsDropEnabled;
    }

   private:
    rapidhashset<QString> tracks;
};
