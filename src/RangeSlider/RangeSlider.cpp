#include "RangeSlider.hpp"

#include <QMouseEvent>
#include <QPainter>

RangeSlider::RangeSlider(QWidget* const parent) :
    QWidget(parent),
    maximumValue(DEFAULT_MAX),
    highVal(DEFAULT_MAX) {
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void RangeSlider::mousePressEvent(QMouseEvent* const event) {
    if ((event->buttons() & Qt::LeftButton) != 0) {
        const u16 xPos = event->pos().x();
        const u16 hit = hitTest(event->pos());

        if (hit == 0) {
            dragging = Dragging::Low;
            dragOffset = xPos - valueToPos(lowVal);
        } else if (hit == 1) {
            dragging = Dragging::High;
            dragOffset = xPos - valueToPos(highVal);
        } else {
            const i32 value = posToValue(xPos);
            const i32 dLow = std::abs(value - lowVal);
            const i32 dHigh = std::abs(value - highVal);

            if (dLow <= dHigh) {
                setLowValue(value);
                dragging = Dragging::Low;
                dragOffset = xPos - valueToPos(lowVal);
            } else {
                setHighValue(value);
                dragging = Dragging::High;
                dragOffset = xPos - valueToPos(highVal);
            }
        }

        event->accept();
    }
}

void RangeSlider::paintEvent(QPaintEvent* const /* event */) {
    auto painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect gRect = grooveRect();
    auto pen = QPen(Qt::gray);
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setBrush(Qt::lightGray);
    painter.drawRoundedRect(gRect, 1, 4);

    const QRect selectionRect = this->selectionRect();
    painter.setBrush(QBrush(palette().highlight()));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(selectionRect, 4, 4);

    const QRect lowHandle = handleRectForValue(lowVal);
    const QRect highHandle = handleRectForValue(highVal);
    painter.setPen(QPen(palette().buttonText(), 1));
    painter.setBrush(palette().button());
    painter.drawEllipse(lowHandle);
    painter.drawEllipse(highHandle);
}

void RangeSlider::mouseMoveEvent(QMouseEvent* const event) {
    if (dragging != Dragging::None) {
        const u16 xPos = event->pos().x();
        const u16 newPos = xPos - dragOffset;
        const i32 value = posToValue(newPos);

        if (dragging == Dragging::Low) {
            setLowValue(std::min<i32>(value, highVal));
        } else if (dragging == Dragging::High) {
            setHighValue(std::max<i32>(value, lowVal));
        }

        event->accept();
    }
}

void RangeSlider::keyPressEvent(QKeyEvent* const event) {
    const i32 step = max<i32>(1, (maximumValue - minimumValue) / 100);
    const i32 bigStep = max<i32>(1, (maximumValue - minimumValue) / 10);

    const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
    bool handled = true;

    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Down:
            if (shift) {
                setHighValue(highVal - step);
            } else {
                setLowValue(lowVal - step);
            }
            break;
        case Qt::Key_Right:
        case Qt::Key_Up:
            if (shift) {
                setHighValue(highVal + step);
            } else {
                setLowValue(lowVal + step);
            }
            break;
        case Qt::Key_PageUp:
            if (shift) {
                setHighValue(highVal + bigStep);
            } else {
                setLowValue(lowVal + bigStep);
            }
            break;
        case Qt::Key_PageDown:
            if (shift) {
                setHighValue(highVal - bigStep);
            } else {
                setLowValue(lowVal - bigStep);
            }
            break;
        case Qt::Key_Home:
            if (shift) {
                setHighValue(maximumValue);
            } else {
                setLowValue(minimumValue);
            }
            break;
        case Qt::Key_End:
            if (shift) {
                setHighValue(maximumValue);
            } else {
                setLowValue(maximumValue);
            }
            break;
        default:
            handled = false;
            break;
    }

    if (handled) {
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void RangeSlider::setMinimum(const i32 minimum) {
    if (minimumValue == minimum) {
        return;
    }

    minimumValue = minimum;
    lowVal = std::max<i32>(lowVal, minimumValue);
    highVal = std::max<i32>(highVal, minimumValue);
    update();
    emit minimumChanged(minimumValue);
}

void RangeSlider::setMaximum(const i32 maximum) {
    if (maximumValue == maximum) {
        return;
    }

    maximumValue = maximum;
    lowVal = std::min<i32>(lowVal, maximumValue);
    highVal = std::min<i32>(highVal, maximumValue);

    update();
    emit maximumChanged(maximumValue);
}

void RangeSlider::setRange(i32 minVal, i32 maxVal) {
    if (minVal > maxVal) {
        std::swap(minVal, maxVal);
    }

    minimumValue = minVal;
    maximumValue = maxVal;
    lowVal = max<i32>(lowVal, minVal);
    highVal = min<i32>(highVal, maxVal);
    lowVal = min<i32>(lowVal, highVal);

    update();
    emit minimumChanged(minimumValue);
    emit maximumChanged(maximumValue);
}

void RangeSlider::setLowValue(i32 value) {
    value = qBound(minimumValue, value, highVal);

    if (lowVal == value) {
        return;
    }

    lowVal = value;
    update();
    emit lowValueChanged(lowVal);
    emit rangeChanged(lowVal, highVal);
}

void RangeSlider::setHighValue(i32 value) {
    value = qBound(lowVal, value, maximumValue);

    if (highVal == value) {
        return;
    }

    highVal = value;
    update();
    emit highValueChanged(highVal);
    emit rangeChanged(lowVal, highVal);
}

[[nodiscard]] auto RangeSlider::grooveRect() const -> QRect {
    const i32 margin = 10;
    return { margin, height() / 2, width() - (2 * margin), 4 };
}

[[nodiscard]] auto RangeSlider::handleRectForValue(const i32 value) const
    -> QRect {
    const QPoint point = posToPoint(value);

    return { point.x() - HANDLE_RADIUS,
             point.y() - HANDLE_RADIUS,
             HANDLE_RADIUS * 2,
             HANDLE_RADIUS * 2 };
}

[[nodiscard]] auto RangeSlider::selectionRect() const -> QRect {
    const QRect gRect = grooveRect();
    const QPoint aPos = posToPoint(lowVal);
    const QPoint bPos = posToPoint(highVal);

    const i32 left = std::min<i32>(aPos.x(), bPos.x());
    const i32 right = std::max<i32>(aPos.x(), bPos.x());
    return { left, gRect.top(), right - left, gRect.height() };
}

[[nodiscard]] auto RangeSlider::valueToPos(const i32 value) const -> i32 {
    const QRect gRect = grooveRect();
    const f64 span = f64(maximumValue - minimumValue);

    if (span <= 0) {
        return gRect.left();
    }

    const f64 frac = f64(value - minimumValue) / span;
    return i32(std::round(gRect.left() + (frac * (gRect.width()))));
}

[[nodiscard]] auto RangeSlider::posToPoint(const i32 value) const -> QPoint {
    const QRect gRect = grooveRect();
    const u16 xPos = valueToPos(value);
    return { xPos, gRect.center().y() };
}

[[nodiscard]] auto RangeSlider::posToValue(const i32 pos) const -> i32 {
    const QRect gRect = grooveRect();
    const f64 span = f64(maximumValue - minimumValue);

    if (span <= 0) {
        return minimumValue;
    }

    f64 frac;
    frac = f64(pos - gRect.left()) / f64(gRect.width());
    frac = std::clamp(frac, 0.0, 1.0);

    const i32 value = i32(std::round(minimumValue + (frac * span)));
    return qBound(minimumValue, value, maximumValue);
}

[[nodiscard]] auto RangeSlider::hitTest(const QPoint& point) const -> i32 {
    const QRect lowValue = handleRectForValue(lowVal).adjusted(-4, -4, 4, 4);
    const QRect highValue = handleRectForValue(highVal).adjusted(-4, -4, 4, 4);

    if (lowValue.contains(point)) {
        return 0;
    }

    if (highValue.contains(point)) {
        return 1;
    }

    return -1;
}
