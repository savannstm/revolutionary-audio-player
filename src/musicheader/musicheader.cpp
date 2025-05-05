#include "musicheader.hpp"

#include "constants.hpp"

#include <QApplication>

void MusicHeader::mousePressEvent(QMouseEvent* event) {
    pressedIndex = as<i8>(logicalIndexAt(event->pos()));
    pressPos = event->pos();
    mousePressed = true;
    isDragging = false;

    QHeaderView::mousePressEvent(event);
}

void MusicHeader::mouseMoveEvent(QMouseEvent* event) {
    if (mousePressed) {
        isDragging = true;
    }

    QHeaderView::mouseMoveEvent(event);
}

void MusicHeader::mouseReleaseEvent(QMouseEvent* event) {
    if (!isDragging && pressedIndex != -1) {
        const u16 posX = event->pos().x();
        const u16 sectionStart = sectionPosition(pressedIndex);
        const u16 sectionEnd = sectionStart + sectionSize(pressedIndex);

        const bool isWithinSection = posX >= sectionStart && posX <= sectionEnd;
        const bool isNearLeftEdge = posX <= sectionStart + HEADER_HANDLE_WIDTH;
        const bool isNearRightEdge = posX >= sectionEnd - HEADER_HANDLE_WIDTH;

        if (isWithinSection && !isNearLeftEdge && !isNearRightEdge) {
            emit headerPressed(pressedIndex, event->button());
        }
    }

    mousePressed = false;
    QHeaderView::mouseReleaseEvent(event);
}