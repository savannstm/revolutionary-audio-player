#include "TrackTreeModel.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "MusicItem.hpp"

TrackTreeModel::TrackTreeModel(QObject* const parent) :
    QStandardItemModel(parent) {
    tracks.reserve(MINIMUM_TRACK_COUNT);
}

void TrackTreeModel::setItem(
    const u16 row,
    const u16 col,
    QStandardItem* const item,
    const bool path
) {
    QStandardItemModel::setItem(row, col, item);

    if (path) {
        tracks.insert(item->text());
    }
}

auto TrackTreeModel::itemFromIndex(const QModelIndex& index) const
    -> MusicItem* {
    return as<MusicItem*>(QStandardItemModel::itemFromIndex(index));
}

auto TrackTreeModel::trackProperty(const u8 column) const -> TrackProperty {
    return TrackProperty(
        headerData(column, Qt::Horizontal, PROPERTY_ROLE).toUInt()
    );
}

auto TrackTreeModel::removeRows(
    const i32 row,
    const i32 count,
    const QModelIndex& parent
) -> bool {
    for (const u16 rowIdx : range(0, count)) {
        for (const u8 column : range(0, TRACK_PROPERTY_COUNT)) {
            if (trackProperty(column) == TrackProperty::Path) {
                tracks.erase(this->item(row + rowIdx, column)->text());
            }
        }
    }

    return QStandardItemModel::removeRows(row, count, parent);
}