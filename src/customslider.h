#ifndef CUSTOM_SLIDER_H
#define CUSTOM_SLIDER_H

#include <QMouseEvent>
#include <QSlider>

class CustomSlider : public QSlider {
    Q_OBJECT

   public:
    using QSlider::QSlider;

   signals:
    void mouseMoved(std::uint32_t value);
    void mousePressed(std::uint32_t value);

   protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->buttons() & Qt::MouseButton::LeftButton) {
            const auto pos = event->pos();
            const double ratio = static_cast<double>(pos.x()) / this->width();
            int value =
                this->minimum() + (ratio * (this->maximum() - this->minimum()));
            value = qBound(this->minimum(), value, this->maximum());
            this->setValue(value);

            emit mouseMoved(value);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            const auto pos = event->pos();
            const double ratio = static_cast<double>(pos.x()) / this->width();
            int value =
                this->minimum() + (ratio * (this->maximum() - this->minimum()));
            value = qBound(this->minimum(), value, this->maximum());
            this->setValue(value);

            emit mousePressed(value);
        }
    }
};
#endif
