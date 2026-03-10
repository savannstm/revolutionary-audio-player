#pragma once

#include "Constants.hpp"
#include "Duration.hpp"
#include "Enums.hpp"
#include "FWD.hpp"
#include "Utils.hpp"

#include <QTableView>

class TrackTable : public QTableView {
    Q_OBJECT

   public:
    explicit TrackTable(QWidget* parent = nullptr);

    [[nodiscard]] constexpr auto model() const -> TrackTableModel* {
        return model_;
    };

    [[nodiscard]] constexpr auto header() const -> TrackTableHeader* {
        return header_;
    };

    void setOpacity(f32 opacity);

    void setCurrentIndex(const QModelIndex& index) {
        playingIndex = index;
        viewport()->update();
    }

    [[nodiscard]] auto currentIndex() const -> QModelIndex {
        return playingIndex;
    }

    [[nodiscard]] auto duration() const -> Duration { return duration_; }

    [[nodiscard]] constexpr auto opacity() const -> f32 { return opacity_; }

    [[nodiscard]] auto rowMetadata(i32 row) const -> TrackMetadata;
    void sortByPath();
    void fill(
        const QStringList& paths,
        const QVariantList& cueOffsets = {},
        bool fromArgs = false
    );
    void fillCUE(
        TrackMetadata& metadata,
        const vector<CUETrack>& tracks,
        const QString& cueFilePath
    );
    auto deselect(i32 index = -1) -> QModelIndex;

    [[nodiscard]] auto columnSettings() const -> ColumnSettingsArray {
        return columnSettings_;
    }

    void setColumnSettings(const ColumnSettingsArray& columnSettings) {
        columnSettings_ = columnSettings;
    }

    void setColumnIndex(const u8 column, const u8 index) {
        columnSettings_[column].index = index;
    }

    void setColumnHidden(const u8 column, const bool hidden) {
        columnSettings_[column].hidden = hidden;
    }

    [[nodiscard]] auto status() const -> TableStatus { return status_; }

    void setStatus(const TableStatus status) { status_ = status; }

    static constexpr u8 TRACK_TABLE_ROW_HEIGHT = 18;

   signals:
    void playingChanged(QModelIndex index);
    void fillingFinished(bool startPlaying);
    void durationChanged(Duration duration);

   protected:
    [[nodiscard]] auto visualRect(const QModelIndex& index) const
        -> QRect override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
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

    inline auto
    makeItemText(u32 row, TrackProperty prop, const TrackMetadata& metadata)
        -> QString;

    ColumnSettingsArray columnSettings_;
    vector<i32> order;

    QPersistentModelIndex playingIndex;

    TrackTableHeader* const header_;
    TrackTableModel* const model_;

    Duration duration_;

    f32 opacity_ = 1.0F;

    TableStatus status_ = TableStatus::Idle;
};
