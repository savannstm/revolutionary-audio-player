#include "playlistview.hpp"

#include "aliases.hpp"
#include "playlisttabbar.hpp"
#include "tracktree.hpp"

PlaylistView::PlaylistView(QWidget* parent) : QWidget(parent) {
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabBar, 0, Qt::AlignLeft);
    layout->addWidget(stackedWidget);

    connect(
        tabBar,
        &PlaylistTabBar::indexChanged,
        this,
        &PlaylistView::changeTabWidget
    );

    connect(
        tabBar,
        &PlaylistTabBar::closeButtonClicked,
        this,
        &PlaylistView::removeTab
    );

    connect(
        tabBar,
        &PlaylistTabBar::tabAdded,
        this,
        &PlaylistView::addTabWidget
    );
}

auto PlaylistView::headerLabels() -> QStringList& {
    return headerLabels_;
}

auto PlaylistView::createTree() -> TrackTree* {
    auto* tree = new TrackTree(this);
    tree->model()->setHorizontalHeaderLabels(headerLabels_);
    return tree;
}

auto PlaylistView::tree(const i8 index) const -> TrackTree* {
    return static_cast<TrackTree*>(stackedWidget->widget(index));
}

void PlaylistView::addTabWidget(i8 index) {
    stackedWidget->insertWidget(index, createTree());
}

auto PlaylistView::addTab(const QString& label) -> i8 {
    const i8 index = static_cast<i8>(stackedWidget->addWidget(createTree()));
    tabBar->addTab(label);
    return index;
}

void PlaylistView::removeTab(const i8 index) {
    TrackTree* trackTree = tree(index);

    stackedWidget->removeWidget(trackTree);
    delete trackTree;

    tabBar->removeTab(index);
}

auto PlaylistView::tabCount() const -> i8 {
    return tabBar->count();
}

void PlaylistView::setCurrentIndex(const i8 index) {
    tabBar->setCurrentIndex(index);
}

auto PlaylistView::currentIndex() const -> i8 {
    return tabBar->currentIndex();
}

void PlaylistView::changeTabWidget(const i8 index) {
    stackedWidget->setCurrentIndex(index);
    emit indexChanged(index);
}

auto PlaylistView::tabText(const i8 index) const -> QString {
    return tabBar->tabText(index);
}