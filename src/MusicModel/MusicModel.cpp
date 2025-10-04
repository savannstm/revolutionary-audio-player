#include "MusicModel.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "MusicItem.hpp"

#include <Qt>

MusicModel::MusicModel(QObject* parent) : QStandardItemModel(parent) {
    tracks.reserve(MINIMUM_TRACK_COUNT);
}

void MusicModel::setItem(
    const u16 row,
    const u16 col,
    QStandardItem* item,
    const bool path
) {
    QStandardItemModel::setItem(row, col, item);

    if (path) {
        tracks.emplace(item->text());
    }
}

auto MusicModel::itemFromIndex(const QModelIndex& index) const -> MusicItem* {
    return as<MusicItem*>(QStandardItemModel::itemFromIndex(index));
}

auto MusicModel::trackProperty(const u8 column) const -> TrackProperty {
    return as<TrackProperty>(
        headerData(column, Qt::Horizontal, PROPERTY_ROLE).toUInt()
    );
}

auto MusicModel::removeRows(
    const i32 row,
    const i32 count,
    const QModelIndex& parent
) -> bool {
    for (const u16 idx : range(0, count)) {
        for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
            if (trackProperty(column) == Title) {
                tracks.erase(this->item(row + idx, column)->text());
            }
        }
    }

    return QStandardItemModel::removeRows(row, count, parent);
}