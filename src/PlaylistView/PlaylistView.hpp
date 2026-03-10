#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QModelIndex>
#include <QWidget>

struct PlayingItem {
    TrackTable* table = nullptr;
    QPersistentModelIndex index;
};

struct AdvanceResult {
    QModelIndex index;
    TrackTable* table = nullptr;
    AdvanceStatus status;
};

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
    enum class TabRemoveMode : u8 {
        Single,
        ToLeft,
        ToRight,
        Other
    };

    enum class Direction : u8 {
        Forward,
        Backward,
    };

    enum class RepeatMode : u8 {
        Off,
        Track,
        Playlist,
    };

    explicit PlaylistView(QWidget* parent = nullptr);

    void setSettings(shared_ptr<Settings> settings_) {
        settings = std::move(settings_);
    }

    void setBackgroundImage(
        u8 index,
        const QImage& image,
        const QString& path
    ) const;
    void setCurrentIndex(i8 index);
    void setPlayingIndex(TrackTable* table, const QModelIndex& index);
    void resetPlayingIndex();
    auto advance(Direction direction) -> AdvanceResult;
    [[nodiscard]] auto repeatMode() const -> RepeatMode;
    void setTabColor(u8 index, const QString& color);
    void setTabLabel(u8 index, const QString& label);

    [[nodiscard]] constexpr auto tabBar() const -> PlaylistTabBar* {
        return tabBar_;
    }

    [[nodiscard]] auto tabCount() const -> u8;
    [[nodiscard]] auto currentIndex() const -> i8;
    [[nodiscard]] auto playingIndex() const -> QModelIndex;
    [[nodiscard]] auto playingTable() const -> TrackTable*;
    [[nodiscard]] auto backgroundImage(u8 index) const -> QLabel*;
    [[nodiscard]] auto currentBackgroundImage() const -> QLabel*;
    [[nodiscard]] auto table(u8 index) const -> TrackTable*;
    [[nodiscard]] auto currentTable() const -> TrackTable*;
    [[nodiscard]] auto tabLabel(u8 index) const -> QString;
    [[nodiscard]] auto tabColor(u8 index) const -> QString;
    [[nodiscard]] auto currentPage() const -> QWidget*;
    [[nodiscard]] auto page(u8 index) const -> QWidget*;
    [[nodiscard]] auto backgroundImagePath(u8 index) const -> QString;
    [[nodiscard]] auto createPage(optional<ColumnSettingsArray> cols = nullopt)
        -> QWidget*;
    [[nodiscard]] auto addTab(
        const QString& label = QString(),
        const ColumnSettingsArray& cols = DEFAULT_COLUMN_SETTINGS
    ) -> u8;
    void removeBackgroundImage(u8 index) const;
    void removePages(TabRemoveMode mode, u8 index);
    void createTabPage(u8 index);
    [[nodiscard]] auto hasBackgroundImage(u8 index) const -> bool;
    void removePage(u8 index);
    void shufflePlaylist();
    void toggleRepeatMode();
    void toggleRandom();

   signals:
    void renameTabRequested(u8 index);
    void closeTabRequested(u8 index);
    void indexChanged(i8 index);
    void tabsRemoved(TabRemoveMode mode, u8 startIndex, u8 count);
    void playingChanged(TrackTable* table, i32 row);
    void trackPressed(const QModelIndex& index);
    void rowsRemoved();
    void rowsInserted();
    void layoutChanged();

   protected:
    // If this is put into a track table, causes a deadlock
    void changeEvent(QEvent* event) override;

   private:
    inline void setTableOpacity(u8 index, f32 opacity) const;

    [[nodiscard]] inline auto tableOpacity(u8 index) const -> f32;

    inline void changePage(i8 index);

    inline auto advanceToNextTable() -> bool;

    static constexpr f32 HALF_TRANSPARENT = 0.5;
    static constexpr f32 BLUR_SIGMA = 10;

    vector<QPersistentModelIndex> shuffledPlaylist;

    shared_ptr<Settings> settings;
    PlayingItem playingData;

    PlaylistTabBar* const tabBar_;
    QStackedWidget* const stackedWidget;
    QVBoxLayout* const layout;

    i32 shuffledPos = 0;

    RepeatMode repeatMode_ = RepeatMode::Off;
    bool random = false;
};