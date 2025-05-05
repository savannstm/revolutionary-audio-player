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
}

auto MusicModel::itemFromIndex(const QModelIndex& index) const -> MusicItem* {
    return as<MusicItem*>(QStandardItemModel::itemFromIndex(index));
}

auto MusicModel::trackProperty(const u8 column) const -> TrackProperty {
    return as<TrackProperty>(
        headerData(column, Qt::Horizontal, PROPERTY_ROLE).toUInt()
    );
}