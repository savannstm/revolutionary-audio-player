#include "musicheader.hpp"

#include <QApplication>

void MusicHeader::mousePressEvent(QMouseEvent* event) {
    pressedIndex = static_cast<i8>(logicalIndexAt(event->pos()));
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

        constexpr u8 handleWidth = 4;

        const bool isWithinSection = posX >= sectionStart && posX <= sectionEnd;
        const bool isNearLeftEdge = posX <= sectionStart + handleWidth;
        const bool isNearRightEdge = posX >= sectionEnd - handleWidth;

        if (isWithinSection && !isNearLeftEdge && !isNearRightEdge) {
            emit headerPressed(pressedIndex, event->button());
        }
    }

    mousePressed = false;
    QHeaderView::mouseReleaseEvent(event);
}