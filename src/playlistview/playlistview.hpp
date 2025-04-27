#pragma once

#include "aliases.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidget>

class PlaylistView : public QWidget {
    Q_OBJECT

   public:
    explicit PlaylistView(QWidget* parent = nullptr);

    auto addTab(const QString& label) -> i8;
    void removeTab(i8 index);
    [[nodiscard]] auto tabCount() const -> i8;
    void setCurrentIndex(i8 index);
    [[nodiscard]] auto currentIndex() const -> i8;
    void createTabPage(i8 index);
    auto createPage() -> QWidget*;
    void setBackgroundImage(i8 index, const QString& path) const;
    [[nodiscard]] auto backgroundImage(i8 index) const -> QLabel*;
    [[nodiscard]] auto currentBackgroundImage() const -> QLabel*;
    [[nodiscard]] auto tree(i8 index) const -> TrackTree*;
    [[nodiscard]] auto currentTree() const -> TrackTree*;
    [[nodiscard]] auto tabText(i8 index) const -> QString;
    [[nodiscard]] auto currentPage() const -> QWidget*;
    [[nodiscard]] auto page(i8 index) const -> QWidget*;
    void removeBackgroundImage(i8 index) const;
    [[nodiscard]] auto backgroundImagePath(i8 index) const -> QString;
    [[nodiscard]] auto hasBackgroundImage(i8 index) const -> bool;
    void removeTabs(RemoveMode mode, i8 index);

   signals:
    void renameTabRequested(i8 index);
    void closeTabRequested(i8 index);
    void indexChanged(i8 index);
    void tabRemoved(i8 index);

   private:
    void changePage(i8 index);

    PlaylistTabBar tabBar = PlaylistTabBar(this);
    QStackedWidget stackedWidget = QStackedWidget(this);
    QVBoxLayout layout = QVBoxLayout(this);
};