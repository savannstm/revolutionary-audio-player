#pragma once

#include "aliases.hpp"
#include "enums.hpp"
#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <QStandardItemModel>

class MusicModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit MusicModel(QObject* parent = nullptr);

    void setItem(u16 row, u16 column, QStandardItem* item, bool path = false);

    [[nodiscard]] auto contains(const QString& path) const -> bool {
        return tracks.contains(path);
    };

    [[nodiscard]] auto itemFromIndex(const QModelIndex& index) const
        -> MusicItem*;
    [[nodiscard]] auto trackProperty(u8 column) const -> TrackProperty;
    auto
    removeRows(i32 row, i32 count, const QModelIndex& parent = QModelIndex())
        -> bool override;

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
