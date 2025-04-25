
#include "musicmodel.hpp"

#include "enums.hpp"
#include "musicitem.hpp"
#include "rapidhasher.hpp"

#include <Qt>

constexpr u8 DEFAULT_SIZE = 128;

MusicModel::MusicModel(QObject* parent) : QStandardItemModel(parent) {
    tracks.reserve(DEFAULT_SIZE);
    rowMetadata_.reserve(DEFAULT_SIZE);
}

void MusicModel::setItem(const u16 row, const u16 col, MusicItem* item) {
    QStandardItemModel::setItem(row, col, item);
    tracks.emplace(&item->path());
}

void MusicModel::sort(const i32 column, const Qt::SortOrder order) {
    rapidhashmap<QString, metadata_array> metadataByPath;
    metadataByPath.reserve(rowCount());

    for (const auto& [row, metadata] : rowMetadata_) {
        metadataByPath.emplace(metadata[TrackProperty::Path], metadata);
    }

    QStandardItemModel::sort(column, order);

    rapidhashmap<u16, metadata_array> newMetadata;
    newMetadata.reserve(rowCount());

    for (u16 row = 0; row < rowCount(); row++) {
        const QString& path = static_cast<MusicItem*>(item(row, 0))->path();
        newMetadata.emplace(row, std::move(metadataByPath.find(path)->second));
    }

    rowMetadata_ = std::move(newMetadata);
}

auto MusicModel::itemFromIndex(const QModelIndex& index) -> MusicItem* {
    return static_cast<MusicItem*>(QStandardItemModel::itemFromIndex(index));
}
