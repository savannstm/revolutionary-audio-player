
#include "musicmodel.hpp"

#include "constants.hpp"
#include "enums.hpp"
#include "rapidhasher.hpp"

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

void MusicModel::clearRowMetadata() {
    rowMetadata.clear();
    tracks.clear();
}

void MusicModel::setRowMetadata(const u16 row, const metadata_array& metadata) {
    rowMetadata[row] = metadata;
}

auto MusicModel::getRowProperty(const u16 row, const TrackProperty property)
    -> const QString& {
    return rowMetadata.find(row)->second[property];
}

auto MusicModel::getRowMetadata(const u16 row) const -> const metadata_array& {
    return rowMetadata.find(row)->second;
}

void MusicModel::sort(const i32 column, const Qt::SortOrder order) {
    rapidhashmap<QString, metadata_array> metadataByPath;
    metadataByPath.reserve(rowCount());

    for (const auto& [row, metadata] : rowMetadata) {
        metadataByPath.emplace(metadata[TrackProperty::Path], metadata);
    }

    QStandardItemModel::sort(column, order);

    rapidhashmap<u32, metadata_array> newMetadata;
    newMetadata.reserve(rowCount());

    for (u16 row = 0; row < rowCount(); row++) {
        const QString& path = static_cast<MusicItem*>(item(row, 0))->getPath();
        newMetadata.emplace(row, std::move(metadataByPath.find(path)->second));
    }

    rowMetadata = std::move(newMetadata);
}

auto MusicModel::contains(const QString& path) const -> bool {
    return tracks.contains(path);
}

void MusicModel::removeRowMetadata(const u16 row) {
    rowMetadata.erase(row);
}