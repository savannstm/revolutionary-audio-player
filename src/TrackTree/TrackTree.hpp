#pragma once

#include "FWD.hpp"
#include "Utils.hpp"

#include <QTreeView>

class TrackTree : public QTreeView {
    Q_OBJECT

   public:
    explicit TrackTree(QWidget* parent = nullptr);

    void setCurrentIndex(const QModelIndex& newIndex);

    [[nodiscard]] constexpr auto currentIndex() const -> QModelIndex {
        return index;
    };

    [[nodiscard]] constexpr auto model() const -> TrackTreeModel* {
        return trackTreeModel;
    };

    [[nodiscard]] constexpr auto header() const -> TrackTreeHeader* {
        return trackTreeHeader;
    };

    void setOpacity(f32 opacity);

    [[nodiscard]] constexpr auto opacity() const -> f32 { return opacity_; }

    [[nodiscard]] auto rowMetadata(u16 row) const -> TrackMetadata;
    void sortByPath();
    void fillTable(
        const QStringList& paths,
        const QList<QVariant>& cueOffsets = {},
        bool fromArgs = false
    );
    void fillTableCUE(
        TrackMetadata& metadata,
        const vector<CUETrack>& tracks,
        const QString& cueFilePath
    );
    auto deselect(i32 index = -1) -> QModelIndex;

   signals:
    void trackSelected(u32 oldIndex, u32 newIndex);
    void fillingFinished(bool startPlaying);

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
    inline void addFile(const TrackMetadata& metadata);
    inline void addFileCUE(
        const CUETrack& track,
        const TrackMetadata& metadata,
        const QString& cueFilePath
    );
    inline void handleHeaderPress(u8 index, Qt::MouseButton button);
    inline void resetSorting(i32 /* unused */, Qt::SortOrder sortOrder);

    vector<u16> order;
    QModelIndex index;
    QPersistentModelIndex persistentIndex;

    TrackTreeHeader* const trackTreeHeader;
    TrackTreeModel* const trackTreeModel;

    f32 opacity_ = 1.0F;
    u16 draggedRow;
};