#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistView(QWidget* parent = nullptr);

    [[nodiscard]] auto addTab(
        const QString& label = QString(),
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties =
            DEFAULT_COLUMN_PROPERTIES,
        const array<bool, TRACK_PROPERTY_COUNT>& columnStates =
            DEFAULT_COLUMN_STATES
    ) -> u8;
    [[nodiscard]] auto tabCount() const -> u8;
    void setCurrentIndex(i8 index);
    [[nodiscard]] auto currentIndex() const -> i8;
    void createTabPage(u8 index);
    [[nodiscard]] auto createPage(
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& columnProperties =
            DEFAULT_COLUMN_PROPERTIES,
        const array<bool, TRACK_PROPERTY_COUNT>& columnStates =
            DEFAULT_COLUMN_STATES
    ) -> QWidget*;
    void setBackgroundImage(u8 index, u16 height, const QString& path) const;
    [[nodiscard]] auto backgroundImage(u8 index) const -> QLabel*;
    [[nodiscard]] auto currentBackgroundImage() const -> QLabel*;
    [[nodiscard]] auto tree(u8 index) const -> TrackTree*;
    [[nodiscard]] auto currentTree() const -> TrackTree*;
    [[nodiscard]] auto tabLabel(u8 index) const -> QString;
    void setTabLabel(u8 index, const QString& label);
    [[nodiscard]] auto currentPage() const -> QWidget*;
    [[nodiscard]] auto page(u8 index) const -> QWidget*;
    void removeBackgroundImage(u8 index) const;
    [[nodiscard]] auto backgroundImagePath(u8 index) const -> QString;
    [[nodiscard]] auto hasBackgroundImage(u8 index) const -> bool;
    void removeTabPages(TabRemoveMode mode, u8 index);
    void removeTabPage(u8 index);

    constexpr auto tabBar() -> PlaylistTabBar* { return tabBar_; }

   signals:
    void renameTabRequested(u8 index);
    void closeTabRequested(u8 index);
    void indexChanged(i8 index);
    void tabsRemoved(TabRemoveMode mode, u8 index, u8 count);

   private:
    void changePage(i8 index);

    PlaylistTabBar* tabBar_ = new PlaylistTabBar(this);
    QStackedWidget* stackedWidget = new QStackedWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
};