#include "musicmodel.hpp"

#include "constants.hpp"
#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <Qt>

MusicModel::MusicModel(QObject* parent) : QStandardItemModel(parent) {
    tracks.reserve(MINIMUM_MUSIC_MODEL_TRACK_COUNT);
}

void MusicModel::setItem(const u16 row, const u16 col, MusicItem* item) {
    QStandardItemModel::setItem(row, col, item);
    tracks.emplace(item->text());
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
    for (i32 i = 0; i < count; i++) {
        tracks.erase(this->item(row + i, 0)->text());
    }

    return QStandardItemModel::removeRows(row, count, parent);
}