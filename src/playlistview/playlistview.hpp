#pragma once

#include "aliases.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

#include <QStackedWidget>
#include <QTabWidget>

class PlaylistView : public QTabWidget {
    Q_OBJECT

   public:
    explicit PlaylistView(QWidget* parent = nullptr);

    auto addPlaylist(bool focus = false, const QString& label = QString())
        -> i32;
    void insertPlaylist(i32 index, const QString& label);
    void setHeaderLabels(const QStringList& labels);
    [[nodiscard]] auto playlistTree(i32 index) const -> TrackTree*;

   private:
    QStringList headerLabels;
    PlaylistTabBar* playlistTabBar = new PlaylistTabBar(this);
    QStackedWidget* stackedWidget = findChild<QStackedWidget*>();
};
