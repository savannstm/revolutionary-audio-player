
#include "musicmodel.hpp"

#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <Qt>

constexpr u8 DEFAULT_SIZE = 128;

MusicModel::MusicModel(QObject* parent) : QStandardItemModel(parent) {
    tracks.reserve(DEFAULT_SIZE);
}

void MusicModel::setItem(const u16 row, const u16 col, MusicItem* item) {
    QStandardItemModel::setItem(row, col, item);
}

auto MusicModel::itemFromIndex(const QModelIndex& index) const -> MusicItem* {
    return static_cast<MusicItem*>(QStandardItemModel::itemFromIndex(index));
}

auto MusicModel::trackProperty(const u8 column) const -> TrackProperty {
    return static_cast<TrackProperty>(
        headerData(column, Qt::Horizontal, PropertyRole).toUInt()
    );
}