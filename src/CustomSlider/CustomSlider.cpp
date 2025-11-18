#include "CustomSlider.hpp"

#include "Aliases.hpp"

#include <QStyle>
#include <QStyleOptionSlider>

void CustomSlider::mouseMoveEvent(QMouseEvent* const event) {
    if ((event->buttons() & Qt::LeftButton) != 0) {
        QStyleOptionSlider opt;
        this->initStyleOption(&opt);

        const QRect groove = this->style()->subControlRect(
            QStyle::CC_Slider,
            &opt,
            QStyle::SC_SliderGroove,
            this
        );
        const QRect handle = this->style()->subControlRect(
            QStyle::CC_Slider,
            &opt,
            QStyle::SC_SliderHandle,
            this
        );

        const u16 sliderMin = groove.x();
        const u16 sliderMax = groove.right() - (handle.width() / 2) + 1;

        const u16 mouseX = event->pos().x();
        const u16 value = QStyle::sliderValueFromPosition(
            this->minimum(),
            this->maximum(),
            mouseX - sliderMin,
            sliderMax - sliderMin,
            opt.upsideDown
        );

        this->setValue(value);
        QSlider::mouseMoveEvent(event);
    }
}

void CustomSlider::mousePressEvent(QMouseEvent* const event) {
    if (event->button() == Qt::LeftButton) {
        QStyleOptionSlider opt;
        this->initStyleOption(&opt);

        const QRect groove = this->style()->subControlRect(
            QStyle::CC_Slider,
            &opt,
            QStyle::SC_SliderGroove,
            this
        );
        const QRect handle = this->style()->subControlRect(
            QStyle::CC_Slider,
            &opt,
            QStyle::SC_SliderHandle,
            this
        );

        const u16 sliderMin = groove.x();
        const u16 sliderMax = groove.right() - (handle.width() / 2) + 1;

        const u16 mouseX = event->pos().x();
        const u16 value = QStyle::sliderValueFromPosition(
            this->minimum(),
            this->maximum(),
            mouseX - sliderMin,
            sliderMax - sliderMin,
            opt.upsideDown
        );

        this->setValue(value);
        QSlider::mousePressEvent(event);
    }
}
