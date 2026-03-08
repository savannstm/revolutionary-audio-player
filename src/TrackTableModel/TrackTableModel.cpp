#include "TrackTableModel.hpp"

#include "Constants.hpp"
#include "Duration.hpp"
#include "Enums.hpp"

#include <QDataStream>
#include <QIODevice>

TrackTableModel::TrackTableModel(QObject* const parent) :
    QAbstractTableModel(parent) {
    rows.reserve(MINIMUM_TRACK_COUNT);
    paths.reserve(MINIMUM_TRACK_COUNT);
}

auto TrackTableModel::rowCount(const QModelIndex& parent) const -> i32 {
    return parent.isValid() ? 0 : i32(rows.size());
}

auto TrackTableModel::columnCount(const QModelIndex& parent) const -> i32 {
    return parent.isValid() ? 0 : TRACK_PROPERTY_COUNT;
}

auto TrackTableModel::data(const QModelIndex& index, const i32 role) const
    -> QVariant {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }

    return rows[index.row()].columns[index.column()];
}

auto TrackTableModel::headerData(
    const i32 section,
    const Qt::Orientation orientation,
    const i32 role
) const -> QVariant {
    if (orientation != Qt::Horizontal) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    if (section < 0 || section >= TRACK_PROPERTY_COUNT) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    const TrackProperty property = TrackProperty(u8(section));
    return headerLabels[u8(property)];
}

auto TrackTableModel::setHeaderData(
    const i32 section,
    const Qt::Orientation orientation,
    const QVariant& value,
    const i32 role
) -> bool {
    if (orientation != Qt::Horizontal) {
        return QAbstractTableModel::setHeaderData(
            section,
            orientation,
            value,
            role
        );
    }

    if (section < 0 || section >= TRACK_PROPERTY_COUNT) {
        return false;
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QAbstractTableModel::setHeaderData(
            section,
            orientation,
            value,
            role
        );
    }

    const auto property = TrackProperty(u8(section));
    headerLabels[u8(property)] = value.toString();

    emit headerDataChanged(orientation, section, section);
    return true;
}

auto TrackTableModel::flags(const QModelIndex& index) const -> Qt::ItemFlags {
    if (!index.isValid()) {
        return Qt::ItemIsDropEnabled;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled;
}

auto TrackTableModel::mimeTypes() const -> QStringList {
    return { ROWS_MIME_TYPE };
}

auto TrackTableModel::mimeData(const QModelIndexList& indexes) const
    -> QMimeData* {
    auto* const mimeData = new QMimeData();

    if (indexes.isEmpty()) {
        return mimeData;
    }

    QList<i32> draggedRows;
    draggedRows.reserve(indexes.size());

    for (const QModelIndex& index : indexes) {
        if (index.isValid()) {
            draggedRows.append(index.row());
        }
    }

    if (draggedRows.isEmpty()) {
        return mimeData;
    }

    ranges::sort(draggedRows);

    i32 writeIndex = 1;
    for (i32 readIndex = 1; readIndex < draggedRows.size(); ++readIndex) {
        if (draggedRows[readIndex] != draggedRows[readIndex - 1]) {
            draggedRows[writeIndex] = draggedRows[readIndex];
            ++writeIndex;
        }
    }
    draggedRows.resize(writeIndex);

    QByteArray payload;
    payload.reserve(i32(sizeof(i32) * (draggedRows.size() + 1)));

    auto stream = QDataStream(&payload, QIODevice::WriteOnly);
    stream << i32(draggedRows.size());

    for (const i32 row : draggedRows) {
        stream << row;
    }

    mimeData->setData(ROWS_MIME_TYPE, payload);
    return mimeData;
}

auto TrackTableModel::dropMimeData(
    const QMimeData* const data,
    const Qt::DropAction action,
    const i32 row,
    const i32 /* column */,
    const QModelIndex& parent
) -> bool {
    if (action == Qt::IgnoreAction) {
        return true;
    }

    if (action != Qt::MoveAction) {
        return false;
    }

    if (data == nullptr || !data->hasFormat(ROWS_MIME_TYPE) ||
        parent.isValid()) {
        return false;
    }

    auto payload = data->data(ROWS_MIME_TYPE);
    if (payload.isEmpty()) {
        return false;
    }

    auto stream = QDataStream(&payload, QIODevice::ReadOnly);

    i32 movedRowsCount = 0;
    stream >> movedRowsCount;

    if (movedRowsCount <= 0) {
        return false;
    }

    QList<i32> sourceRows;
    sourceRows.reserve(movedRowsCount);

    for (i32 idx = 0; idx < movedRowsCount; ++idx) {
        i32 sourceRow = -1;
        stream >> sourceRow;
        sourceRows.append(sourceRow);
    }

    ranges::sort(sourceRows);

    const i32 firstRow = sourceRows.front();
    const i32 lastRow = sourceRows.back();

    if (firstRow < 0 || lastRow >= rows.size()) {
        return false;
    }

    for (const i32 idx : range(1, i32(sourceRows.size()))) {
        if (sourceRows[idx] != sourceRows[idx - 1] + 1) {
            return false;
        }
    }

    i32 destinationRow = row;
    if (destinationRow < 0) {
        destinationRow = i32(rows.size());
    }

    destinationRow = clamp(destinationRow, 0, i32(rows.size()));

    if (destinationRow >= firstRow && destinationRow <= lastRow + 1) {
        return false;
    }

    return moveRows({}, firstRow, movedRowsCount, {}, destinationRow);
}

auto TrackTableModel::supportedDragActions() const -> Qt::DropActions {
    return Qt::MoveAction;
}

auto TrackTableModel::supportedDropActions() const -> Qt::DropActions {
    return Qt::MoveAction;
}

void TrackTableModel::sort(const i32 column, const Qt::SortOrder order) {
    if (column == -1) {
        // TODO: Sort by path
        return;
    }

    if (rows.size() < 2) {
        return;
    }

    emit layoutAboutToBeChanged();

    auto perm = QList<i32>(rows.size());
    ranges::iota(perm, 0);

    const auto compareByOrder =
        [order](const auto& lhs, const auto& rhs) -> bool {
        return order == Qt::AscendingOrder ? lhs < rhs : lhs > rhs;
    };

    ranges::stable_sort(perm, [&](const i32 a, const i32 b) -> bool {
        const QString& lhs = rows[a].columns[column];
        const QString& rhs = rows[b].columns[column];

        switch (TrackProperty(column)) {
            case TrackProperty::Duration:
                return compareByOrder(
                    Duration::stringToSeconds(lhs).value_or(0),
                    Duration::stringToSeconds(rhs).value_or(0)
                );
            case TrackProperty::Order:
            case TrackProperty::TrackNumber:
            case TrackProperty::Channels:
            case TrackProperty::SampleRate:
            case TrackProperty::Bitrate:
            case TrackProperty::BPM:
            case TrackProperty::TotalDiscs:
            case TrackProperty::TotalTracks:
                return compareByOrder(lhs.toUInt(), rhs.toUInt());
            default:
                return compareByOrder(lhs, rhs);
        }
    });

    QModelIndexList fromIndex;
    QModelIndexList toIndex;

    for (i32 newRow = 0; newRow < rows.size(); ++newRow) {
        const i32 oldRow = perm[newRow];

        if (oldRow == newRow) {
            continue;
        }

        for (i32 col = 0; col < TRACK_PROPERTY_COUNT; ++col) {
            fromIndex.append(createIndex(oldRow, col));
            toIndex.append(createIndex(newRow, col));
        }
    }

    auto sorted = QList<TrackRow>(rows.size());
    for (i32 newRow = 0; newRow < rows.size(); ++newRow) {
        sorted[newRow] = rows[perm[newRow]];
    }
    rows = std::move(sorted);

    changePersistentIndexList(fromIndex, toIndex);
    emit layoutChanged();
}

void TrackTableModel::appendRow(TrackRow row) {
    const i32 newRow = i32(rows.size());

    beginInsertRows({}, newRow, newRow);
    if (!row.columns[u8(TrackProperty::Path)].isEmpty()) {
        paths.insert(row.columns[u8(TrackProperty::Path)]);
    }

    rows.append(std::move(row));
    endInsertRows();
}

void TrackTableModel::setText(
    const i32 row,
    const u8 column,
    const QString& text
) {
    if (column == u8(TrackProperty::Path) && !text.isEmpty()) {
        paths.insert(text);
    }

    rows[row].columns[column] = text;
    const QModelIndex idx = index(row, column);
    emit dataChanged(idx, idx, { Qt::DisplayRole });
}

auto TrackTableModel::item(const i32 row, const u8 column) const
    -> const QString& {
    return rows[row].columns[column];
}

auto TrackTableModel::removeRows(
    const i32 row,
    const i32 count,
    const QModelIndex& parent
) -> bool {
    beginRemoveRows({}, row, row + count - 1);

    for (i32 targetRow = row; targetRow < row + count; ++targetRow) {
        const QString& path = rows[targetRow].columns[u8(TrackProperty::Path)];
        if (!path.isEmpty()) {
            paths.erase(path);
        }
    }

    rows.remove(row, count);
    endRemoveRows();
    return true;
}

auto TrackTableModel::moveRows(
    const QModelIndex& sourceParent,
    const i32 sourceRow,
    const i32 count,
    const QModelIndex& destinationParent,
    const i32 destinationRow
) -> bool {
    if (!beginMoveRows(
            {},
            sourceRow,
            sourceRow + count - 1,
            {},
            destinationRow
        )) {
        return false;
    }

    auto stash = QList<TrackRow>(
        rows.begin() + sourceRow,
        rows.begin() + sourceRow + count
    );
    rows.remove(sourceRow, count);

    const i32 insertAt =
        destinationRow > sourceRow ? destinationRow - count : destinationRow;
    for (i32 i = 0; i < stash.size(); ++i) {
        rows.insert(insertAt + i, stash[i]);
    }

    endMoveRows();
    return true;
}
