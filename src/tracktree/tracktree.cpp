#include "tracktree.hpp"

#include <QMouseEvent>

TrackTree::TrackTree() {
    musicHeader = new MusicHeader(Qt::Orientation::Horizontal, this);
    musicModel = new MusicModel(this);
    setHeader(musicHeader);
    setModel(musicModel);
}

TrackTree::TrackTree(QWidget* parent) : QTreeView(parent) {
    musicHeader = new MusicHeader(Qt::Orientation::Horizontal, this);
    musicModel = new MusicModel(this);
    setHeader(musicHeader);
    setModel(musicModel);
}

void TrackTree::mousePressEvent(QMouseEvent* event) {
    const QModelIndex newIndex = indexAt(event->pos());
    if (!newIndex.isValid()) {
        return;
    }

    emit pressed(index, newIndex);
    index = newIndex;
    QTreeView::mousePressEvent(event);
}

auto TrackTree::currentIndex() const -> QModelIndex {
    return index;
}

void TrackTree::setCurrentIndex(const QModelIndex& newIndex) {
    index = newIndex;
    QTreeView::setCurrentIndex(index);
}

auto TrackTree::header() -> MusicHeader* {
    return musicHeader;
}

auto TrackTree::model() -> MusicModel* {
    return musicModel;
}
