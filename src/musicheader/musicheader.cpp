#include "musicheader.hpp"

MusicHeader::MusicHeader(Qt::Orientation orientation, QWidget* parent) :
    QHeaderView(orientation, parent) {}

void MusicHeader::mousePressEvent(QMouseEvent* event) {
    emit headerPressed(logicalIndexAt(event->pos()), event->button());
    QHeaderView::mousePressEvent(event);
}