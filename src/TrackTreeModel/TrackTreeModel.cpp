#include "TrackTreeModel.hpp"

#include "Constants.hpp"
#include "TrackTreeItem.hpp"

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
    -> TrackTreeItem* {
    return as<TrackTreeItem*>(QStandardItemModel::itemFromIndex(index));
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

auto TrackTreeModel::moveRows(
    const QModelIndex& srcParent,
    i32 src,
    i32 count,
    const QModelIndex& destParent,
    i32 dest
) -> bool {}

[[nodiscard]] auto TrackTreeModel::item(const i32 row, const i32 column) const
    -> TrackTreeItem* {
    return as<TrackTreeItem*>(QStandardItemModel::item(row, column));
}

[[nodiscard]] auto TrackTreeModel::flags(const QModelIndex& index) const
    -> Qt::ItemFlags {
    const Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);

    if (!index.isValid()) {
        return Qt::ItemIsDropEnabled | defaultFlags;
    }

    return defaultFlags & ~Qt::ItemIsDropEnabled;
}
