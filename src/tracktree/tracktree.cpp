#include "tracktree.hpp"

#include <QMouseEvent>

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