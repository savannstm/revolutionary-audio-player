#pragma once

#include "Aliases.hpp"

#include <QHeaderView>

class TrackTreeHeader : public QHeaderView {
    Q_OBJECT

   public:
    using QHeaderView::QHeaderView;

   signals:
    void headerPressed(u8 index, Qt::MouseButton button);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    QPoint pressPos;
    i8 pressedIndex = -1;
    bool isDragging = false;
    bool mousePressed = false;
};