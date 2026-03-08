#pragma once

#include "Aliases.hpp"
#include "Duration.hpp"
#include "Enums.hpp"

#include <QAbstractTableModel>
#include <QMimeData>

struct TrackRow {
    array<QString, TRACK_PROPERTY_COUNT> columns;
    QString cueFilePath;
    u32 cueOffset;
};

class TrackTableModel : public QAbstractTableModel {
    Q_OBJECT

   public:
    inline static const QString ROWS_MIME_TYPE = u"tracktablerows"_s;

    explicit TrackTableModel(QObject* parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex()) const
        -> i32 override;

    [[nodiscard]] auto columnCount(
        const QModelIndex& parent = QModelIndex()
    ) const -> i32 override;

    [[nodiscard]] auto data(
        const QModelIndex& index,
        i32 role = Qt::DisplayRole
    ) const -> QVariant override;

    [[nodiscard]] auto headerData(
        i32 section,
        Qt::Orientation orientation,
        i32 role = Qt::DisplayRole
    ) const -> QVariant override;

    auto setHeaderData(
        i32 section,
        Qt::Orientation orientation,
        const QVariant& value,
        i32 role = Qt::EditRole
    ) -> bool override;

    [[nodiscard]] auto flags(const QModelIndex& index) const
        -> Qt::ItemFlags override;

    [[nodiscard]] auto mimeTypes() const -> QStringList override;
    [[nodiscard]] auto mimeData(const QModelIndexList& indexes) const
        -> QMimeData* override;
    auto dropMimeData(
        const QMimeData* data,
        Qt::DropAction action,
        i32 row,
        i32 column,
        const QModelIndex& parent
    ) -> bool override;
    [[nodiscard]] auto supportedDragActions() const -> Qt::DropActions override;
    [[nodiscard]] auto supportedDropActions() const -> Qt::DropActions override;

    void sort(i32 column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void appendRow(TrackRow row);

    void setCUEInfo(i32 row, const QString& cueFilePath, u32 cueOffset) {
        rows[row].cueFilePath = cueFilePath;
        rows[row].cueOffset = cueOffset;
    }

    auto cueOffset(i32 row) const -> u32 { return rows[row].cueOffset; }

    auto cueFilePath(i32 row) const -> const QString& {
        return rows[row].cueFilePath;
    }

    auto duration(i32 row) const -> u32 {
        return Duration::stringToSeconds(
                   rows[row].columns[u8(TrackProperty::Duration)]
        )
            .value();
    }

    void setOrder(i32 row, const QString& order) {
        rows[row].columns[u8(TrackProperty::Order)] = order;
    }

    void setText(i32 row, u8 column, const QString& text);
    [[nodiscard]] auto item(i32 row, u8 column) const -> const QString&;

    auto
    removeRows(i32 row, i32 count, const QModelIndex& parent = QModelIndex())
        -> bool override;

    auto moveRows(
        const QModelIndex& sourceParent,
        i32 sourceRow,
        i32 count,
        const QModelIndex& destinationParent,
        i32 destinationRow
    ) -> bool override;

    [[nodiscard]] auto contains(const QString& path) const -> bool {
        return paths.contains(path);
    }

   private:
    QList<TrackRow> rows;
    HashSet<QString> paths;
    array<QString, TRACK_PROPERTY_COUNT> headerLabels = {};
};
