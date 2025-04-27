#include "tracktree.hpp"

#include <QMouseEvent>

TrackTree::TrackTree() {
    setHeader(musicHeader);
    setModel(musicModel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIndentation(1);
    setSortingEnabled(true);
}

TrackTree::TrackTree(QWidget* parent) : QTreeView(parent) {
    setHeader(musicHeader);
    setModel(musicModel);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setIndentation(1);
    setSortingEnabled(true);
}

void TrackTree::mouseDoubleClickEvent(QMouseEvent* event) {
    const QModelIndex newIndex = indexAt(event->pos());
    if (!newIndex.isValid()) {
        return;
    }

    emit trackSelected(index.row(), newIndex.row());
    index = newIndex;
    setCurrentIndex(newIndex);
    QTreeView::mouseDoubleClickEvent(event);
}

void TrackTree::setCurrentIndex(const QModelIndex& newIndex) {
    index = newIndex;
    QTreeView::setCurrentIndex(index);
}
