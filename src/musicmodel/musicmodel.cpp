
#include "musicmodel.hpp"

#include "constants.hpp"

#include <Qt>

constexpr u8 DEFAULT_SIZE = 128;

MusicModel::MusicModel(QObject* parent) : QStandardItemModel(parent) {
    tracks.reserve(DEFAULT_SIZE);
    rowMetadata.reserve(DEFAULT_SIZE);
}

void MusicModel::setItem(const u16 row, const u16 col, MusicItem* item) {
    QStandardItemModel::setItem(row, col, item);
    tracks.emplace(item->getPath());
}

void MusicModel::setRowMetadata(
    const u16 row,
    const array<string, PROPERTY_COUNT>& metadata
) {
    rowMetadata[row] = metadata;
}

auto MusicModel::getRowMetadata(const u16 row) const
    -> const array<string, PROPERTY_COUNT>& {
    return rowMetadata.find(row)->second;
}

auto MusicModel::getRowPath(const u16 row) const -> const string& {
    return rowMetadata.find(row)->second[TrackProperty::Path];
}

void MusicModel::sort(const i32 column, const Qt::SortOrder order) {
    hashmap<string, array<string, PROPERTY_COUNT>> metadataByPath;
    metadataByPath.reserve(rowCount());

    for (const auto& [row, metadata] : rowMetadata) {
        metadataByPath.emplace(metadata[TrackProperty::Path], metadata);
    }

    QStandardItemModel::sort(column, order);

    hashmap<u32, array<string, PROPERTY_COUNT>> newMetadata;
    newMetadata.reserve(rowCount());

    for (i32 row = 0; row < rowCount(); row++) {
        const string& path = static_cast<MusicItem*>(item(row, 0))->getPath();
        newMetadata.emplace(row, std::move(metadataByPath.find(path)->second));
    }

    rowMetadata = std::move(newMetadata);
}

auto MusicModel::contains(const path& path) const -> bool {
    return tracks.contains(path);
}
