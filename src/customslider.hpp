#pragma once

#include <QMouseEvent>
#include <QSlider>

#include "type_aliases.hpp"

class CustomSlider : public QSlider {
    Q_OBJECT

   public:
    using QSlider::QSlider;

   signals:
    void mouseMoved(u32 value);
    void mousePressed(u32 value);

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
};