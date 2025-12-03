#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

#include <QWidget>

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
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

    void setTabColor(u8 index, const QString& color);

    void setTabLabel(u8 index, const QString& label);

    [[nodiscard]] constexpr auto tabBar() const -> PlaylistTabBar* {
        return tabBar_;
    }

    [[nodiscard]] auto tabCount() const -> u8;

    [[nodiscard]] auto currentIndex() const -> i8;

    [[nodiscard]] auto backgroundImage(u8 index) const -> QLabel*;

    [[nodiscard]] auto currentBackgroundImage() const -> QLabel*;

    [[nodiscard]] auto tree(u8 index) const -> TrackTree*;

    [[nodiscard]] auto currentTree() const -> TrackTree*;

    [[nodiscard]] auto tabLabel(u8 index) const -> QString;

    [[nodiscard]] auto tabColor(u8 index) const -> QString;

    [[nodiscard]] auto currentPage() const -> QWidget*;

    [[nodiscard]] auto page(u8 index) const -> QWidget*;

    [[nodiscard]] auto backgroundImagePath(u8 index) const -> QString;

    [[nodiscard]] auto createPage(
        optional<array<TrackProperty, TRACK_PROPERTY_COUNT>> defaultColumns =
            nullopt
    ) -> QWidget*;

    [[nodiscard]] auto addTab(
        const QString& label = QString(),
        const array<TrackProperty, TRACK_PROPERTY_COUNT>& defaultColumns =
            DEFAULT_COLUMN_PROPERTIES
    ) -> u8;

    void removeBackgroundImage(u8 index) const;
    void removePages(TabRemoveMode mode, u8 index);

    void createTabPage(u8 index);

    [[nodiscard]] auto hasBackgroundImage(u8 index) const -> bool;

    void removePage(u8 index);

   signals:
    void renameTabRequested(u8 index);
    void closeTabRequested(u8 index);
    void indexChanged(i8 index);
    void tabsRemoved(TabRemoveMode mode, u8 startIndex, u8 count);

   protected:
    // If this is put into a track tree, causes a deadlock
    void changeEvent(QEvent* event) override;

   private:
    inline void setTreeOpacity(u8 index, f32 opacity) const;

    [[nodiscard]] inline auto treeOpacity(u8 index) const -> f32;

    inline void changePage(i8 index);

    shared_ptr<Settings> settings;
    PlaylistTabBar* const tabBar_;
    QStackedWidget* const stackedWidget;
    QVBoxLayout* const layout;
};