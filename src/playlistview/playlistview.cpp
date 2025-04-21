#include "playlistview.hpp"

#include <QString>
#include <QVBoxLayout>

PlaylistView::PlaylistView(QWidget* parent) : QTabWidget(parent) {
    setTabBar(playlistTabBar);

    connect(playlistTabBar, &PlaylistTabBar::playlistAdded, this, [this] {
        addPlaylist(true);
    });
}

auto PlaylistView::addPlaylist(const bool focus, const QString& label) -> i32 {
    const QString playlistName =
        label.isEmpty() ? tr("Playlist %1").arg(count()) : label;

    const i32 insertIndex = count() - 1;
    playlistTabBar->insertPlaylistTab(insertIndex, playlistName);

    auto* tree = new TrackTree(stackedWidget);
    tree->model()->setHorizontalHeaderLabels(headerLabels);

    stackedWidget->addWidget(tree);

    if (focus) {
        setCurrentIndex(insertIndex);
    }

    return insertIndex;
}

void PlaylistView::insertPlaylist(i32 index, const QString& label) {
    playlistTabBar->insertPlaylistTab(index, label);

    auto* tree = new TrackTree(stackedWidget);
    tree->model()->setHorizontalHeaderLabels(headerLabels);

    stackedWidget->insertWidget(index, tree);
}

void PlaylistView::setHeaderLabels(const QStringList& labels) {
    headerLabels = labels;
}

auto PlaylistView::playlistTree(i32 index) const -> TrackTree* {
    return static_cast<TrackTree*>(stackedWidget->widget(index));
}
