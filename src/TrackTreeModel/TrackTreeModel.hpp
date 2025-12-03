#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QStandardItemModel>

class TrackTreeModel : public QStandardItemModel {
    Q_OBJECT

   public:
    explicit TrackTreeModel(QObject* parent = nullptr);

    [[nodiscard]] auto item(i32 row, i32 column = 0) const -> TrackTreeItem*;

    [[nodiscard]] auto itemFromIndex(const QModelIndex& index) const
        -> TrackTreeItem*;

    [[nodiscard]] auto trackProperty(u8 column) const -> TrackProperty;
    auto
    removeRows(i32 row, i32 count, const QModelIndex& parent = QModelIndex())
        -> bool override;

    void setItem(u16 row, u16 column, QStandardItem* item, bool path = false);

    [[nodiscard]] auto contains(const QString& path) const -> bool {
        return tracks.contains(path);
    };

    auto moveRows(
        const QModelIndex& srcParent,
        i32 src,
        i32 count,
        const QModelIndex& destParent,
        i32 dest
    ) -> bool override;

   protected:
    [[nodiscard]] auto flags(const QModelIndex& index) const
        -> Qt::ItemFlags override;

   private:
    HashSet<QString> tracks;
};
