#pragma once

#include "Enums.hpp"
#include "MusicHeader.hpp"
#include "TrackTreeModel.hpp"

#include <QTreeView>

struct CUETrack {
    QString title;
    QString artist;
    QString trackNumber;
    u16 offset;
};

// TODO: Customizable opacity

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

    [[nodiscard]] constexpr auto header() const -> MusicHeader* {
        return musicHeader;
    };

    [[nodiscard]] auto rowMetadata(u16 row) const
        -> HashMap<TrackProperty, QString>;
    void sortByPath();
    void fillTable(
        const QStringList& paths,
        const QList<QVariant>& cueOffsets = {},
        bool fromArgs = false
    );
    void fillTableCUE(
        HashMap<TrackProperty, QString>& metadata,
        const QList<CUETrack>& tracks,
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
    inline void addFile(const HashMap<TrackProperty, QString>& metadata);
    inline void addFileCUE(
        const CUETrack& track,
        const HashMap<TrackProperty, QString>& metadata,
        const QString& cueFilePath
    );
    inline void handleHeaderPress(u8 index, Qt::MouseButton button);
    inline void resetSorting(i32 /* unused */, Qt::SortOrder sortOrder);

    QModelIndex index;
    MusicHeader* const musicHeader = new MusicHeader(Qt::Horizontal, this);
    TrackTreeModel* const trackTreeModel = new TrackTreeModel(this);

    vector<u16> order;

    u16 draggedRow;
};