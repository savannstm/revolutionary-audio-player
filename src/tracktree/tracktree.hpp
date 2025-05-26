#pragma once

#include "enums.hpp"
#include "musicheader.hpp"
#include "musicmodel.hpp"

#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    explicit TrackTree(QWidget* parent = nullptr);

    void setCurrentIndex(const QModelIndex& newIndex);

    [[nodiscard]] constexpr auto currentIndex() const -> QModelIndex {
        return index;
    };

    [[nodiscard]] constexpr auto model() const -> MusicModel* {
        return musicModel;
    };

    [[nodiscard]] constexpr auto header() const -> MusicHeader* {
        return musicHeader;
    };

    [[nodiscard]] auto rowMetadata(u16 row) const
        -> HashMap<TrackProperty, QString>;
    void sortByPath();
    void fillTable(const QStringList& paths);
    auto deselect(i32 index = -1) -> QModelIndex;

   signals:
    void metadataReceived(
        const QString& filePath,
        const HashMap<TrackProperty, QString>& metadata
    );
    void finishedFilling();
    void trackSelected(u32 oldIndex, u32 newIndex);

   protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

   private:
    inline void reselectCurrentElement(
        const QItemSelection& /* unused */,
        const QItemSelection& deselected
    );
    inline void postFill();
    inline void resizeColumnsToContents();
    inline void addFile(
        const QString& filePath,
        const HashMap<TrackProperty, QString>& metadata
    );
    inline void handleHeaderPress(u8 index, Qt::MouseButton button);
    inline void resetSorting(i32 /* unused */, Qt::SortOrder sortOrder);

    QModelIndex index;
    MusicHeader* musicHeader = new MusicHeader(Qt::Horizontal, this);
    MusicModel* musicModel = new MusicModel(this);

    vector<u16> customNumbers;

    u16 draggedRow;
};