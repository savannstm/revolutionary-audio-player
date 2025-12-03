#pragma once

#include <QSlider>

class CustomSlider : public QSlider {
    Q_OBJECT

   public:
    using QSlider::QSlider;

   protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
};