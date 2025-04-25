#pragma once

#include "aliases.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

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

    void addTabWidget(i8 index);

    auto createTree() -> TrackTree*;

    [[nodiscard]] auto tree(i8 index) const -> TrackTree*;

    [[nodiscard]] auto headerLabels() -> QStringList&;

    [[nodiscard]] auto tabText(i8 index) const -> QString;

   signals:
    void renameTabRequested(i8 index);
    void closeTabRequested(i8 index);
    void indexChanged(i8 index);

   private:
    void changeTabWidget(i8 index);

    QStringList headerLabels_ = { "Title", "Artist", "Track Number" };
    PlaylistTabBar* tabBar = new PlaylistTabBar(this);
    QStackedWidget* stackedWidget = new QStackedWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
};