#ifndef CUSTOM_SLIDER_HPP
#define CUSTOM_SLIDER_HPP

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
    void mouseMoveEvent(QMouseEvent* event) override {
        if ((event->buttons() & Qt::MouseButton::LeftButton) != 0) {
            const auto pos = event->pos();
            const double ratio = static_cast<double>(pos.x()) / this->width();
            i32 value =
                static_cast<i32>(this->minimum() +
                                 (ratio * (this->maximum() - this->minimum())));
            value = qBound(this->minimum(), value, this->maximum());
            this->setValue(value);

            emit mouseMoved(value);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            const auto pos = event->pos();
            const double ratio = static_cast<double>(pos.x()) / this->width();
            i32 value =
                static_cast<i32>(this->minimum() +
                                 (ratio * (this->maximum() - this->minimum())));
            value = qBound(this->minimum(), value, this->maximum());
            this->setValue(value);

            emit mousePressed(value);
        }
    }
};
#endif
