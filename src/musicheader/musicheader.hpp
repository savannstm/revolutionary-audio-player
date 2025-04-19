#pragma once

#include "aliases.hpp"

#include <QHeaderView>
#include <QMouseEvent>

class MusicHeader : public QHeaderView {
    Q_OBJECT

   public:
    explicit MusicHeader(
        Qt::Orientation orientation,
        QWidget* parent = nullptr
    );

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