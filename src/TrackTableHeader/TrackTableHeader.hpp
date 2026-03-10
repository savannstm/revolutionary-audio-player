#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QHeaderView>

class TrackTableHeader : public QHeaderView {
    Q_OBJECT

   public:
    explicit TrackTableHeader(Qt::Orientation orientation, TrackTable* table);

   signals:
    void headerPressed(u8 index, Qt::MouseButton button);

   protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    static constexpr u8 HEADER_HANDLE_WIDTH = 4;

    QPoint pressPos;
    i8 pressedIndex = -1;
    bool isDragging = false;
    bool mousePressed = false;
};