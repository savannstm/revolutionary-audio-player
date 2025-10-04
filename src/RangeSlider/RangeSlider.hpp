#pragma once

#include "Aliases.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QWidget>
#include <algorithm>
#include <cmath>

constexpr u16 DEFAULT_MAX = 100;

class RangeSlider : public QWidget {
    Q_OBJECT

   public:
    explicit RangeSlider(QWidget* parent = nullptr) :
        QWidget(parent),
        maximumValue(DEFAULT_MAX),
        highVal(DEFAULT_MAX) {
        setFocusPolicy(Qt::StrongFocus);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }

    [[nodiscard]] auto minimum() const -> u16 { return minimumValue; }

    [[nodiscard]] auto maximum() const -> u16 { return maximumValue; }

    [[nodiscard]] auto lowValue() const -> u16 { return lowVal; }

    [[nodiscard]] auto highValue() const -> u16 { return highVal; }

    void setMinimum(const u16 minimum) {
        if (minimumValue == minimum) {
            return;
        }

        minimumValue = minimum;
        lowVal = std::max(lowVal, minimumValue);
        highVal = std::max(highVal, minimumValue);
        update();
        emit minimumChanged(minimumValue);
    }

    void setMaximum(const u16 maximum) {
        if (maximumValue == maximum) {
            return;
        }

        maximumValue = maximum;
        lowVal = std::min(lowVal, maximumValue);
        highVal = std::min(highVal, maximumValue);

        update();
        emit maximumChanged(maximumValue);
    }

    void setRange(u16 min, u16 max) {
        if (min > max) {
            std::swap(min, max);
        }

        minimumValue = min;
        maximumValue = max;
        lowVal = std::max(lowVal, min);
        highVal = std::min(highVal, max);
        lowVal = std::min(lowVal, highVal);

        update();
        emit minimumChanged(minimumValue);
        emit maximumChanged(maximumValue);
    }

    void setLowValue(u16 value) {
        value = qBound(minimumValue, value, highVal);

        if (lowVal == value) {
            return;
        }

        lowVal = value;
        update();
        emit lowValueChanged(lowVal);
        emit rangeChanged(lowVal, highVal);
    }

    void setHighValue(u16 value) {
        value = qBound(lowVal, value, maximumValue);

        if (highVal == value) {
            return;
        }

        highVal = value;
        update();
        emit highValueChanged(highVal);
        emit rangeChanged(lowVal, highVal);
    }

   signals:
    void minimumChanged(u16 value);
    void maximumChanged(u16 value);
    void lowValueChanged(u16 value);
    void highValueChanged(u16 value);
    void rangeChanged(u16 low, u16 high);

   protected:
    [[nodiscard]] auto sizeHint() const -> QSize override {
        return { 160, 48 };
    }

    void paintEvent(QPaintEvent* /* event */) override {
        QPainter painter = QPainter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRect gRect = grooveRect();
        QPen pen = QPen(Qt::gray);
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

    void mousePressEvent(QMouseEvent* event) override {
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
                const u16 value = posToValue(xPos);
                const u16 dLow = std::abs(value - lowVal);
                const u16 dHigh = std::abs(value - highVal);

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

    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging != Dragging::None) {
            const u16 xPos = event->pos().x();
            const u16 newPos = xPos - dragOffset;
            const u16 value = posToValue(newPos);

            if (dragging == Dragging::Low) {
                setLowValue(qMin(value, highVal));
            } else if (dragging == Dragging::High) {
                setHighValue(qMax(value, lowVal));
            }

            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* /* event */) override {
        dragging = None;
    }

    void keyPressEvent(QKeyEvent* event) override {
        const u16 step = qMax(1, (maximumValue - minimumValue) / 100);
        const u16 bigStep = qMax(1, (maximumValue - minimumValue) / 10);

        bool handled = true;
        switch (event->key()) {
            case Qt::Key_Left:
            case Qt::Key_Down:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                    setHighValue(highVal - step);
                } else {
                    setLowValue(lowVal - step);
                }
                break;
            case Qt::Key_Right:
            case Qt::Key_Up:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                    setHighValue(highVal + step);
                } else {
                    setLowValue(lowVal + step);
                }
                break;
            case Qt::Key_PageUp:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                    setHighValue(highVal + bigStep);
                } else {
                    setLowValue(lowVal + bigStep);
                }
                break;
            case Qt::Key_PageDown:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                    setHighValue(highVal - bigStep);
                } else {
                    setLowValue(lowVal - bigStep);
                }
                break;
            case Qt::Key_Home:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
                    setHighValue(maximumValue);
                } else {
                    setLowValue(minimumValue);
                }
                break;
            case Qt::Key_End:
                if ((event->modifiers() & Qt::ShiftModifier) != 0) {
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

   private:
    enum Dragging : u8 {
        None = 0,
        Low = 1,
        High = 2
    };

    [[nodiscard]] auto grooveRect() const -> QRect {
        const u16 margin = 10;
        return { margin, height() / 2, width() - (2 * margin), 4 };
    }

    [[nodiscard]] auto handleRectForValue(const u16 value) const -> QRect {
        const u16 radius = handleRadius;
        const QPoint point = posToPoint(value);

        return { point.x() - radius,
                 point.y() - radius,
                 radius * 2,
                 radius * 2 };
    }

    [[nodiscard]] auto selectionRect() const -> QRect {
        const QRect gRect = grooveRect();
        const QPoint aPos = posToPoint(lowVal);
        const QPoint bPos = posToPoint(highVal);

        const u16 left = qMin(aPos.x(), bPos.x());
        const u16 right = qMax(aPos.x(), bPos.x());
        return { left, gRect.top(), right - left, gRect.height() };
    }

    [[nodiscard]] auto valueToPos(const u16 value) const -> u16 {
        const QRect gRect = grooveRect();
        const f64 span = f64(maximumValue - minimumValue);

        if (span <= 0) {
            return gRect.left();
        }

        const f64 frac = f64(value - minimumValue) / span;
        return u16(std::round(gRect.left() + (frac * (gRect.width()))));
    }

    [[nodiscard]] auto posToPoint(const u16 value) const -> QPoint {
        QRect gRect = grooveRect();
        u16 xPos = valueToPos(value);
        return { xPos, gRect.center().y() };
    }

    [[nodiscard]] auto posToValue(const u16 pos) const -> u16 {
        const QRect gRect = grooveRect();
        const f64 span = f64(maximumValue - minimumValue);

        if (span <= 0) {
            return minimumValue;
        }

        f64 frac;
        frac = f64(pos - gRect.left()) / f64(gRect.width());
        frac = std::clamp(frac, 0.0, 1.0);

        const u16 value = u16(std::round(minimumValue + (frac * span)));
        return qBound(minimumValue, value, maximumValue);
    }

    [[nodiscard]] auto hitTest(const QPoint& point) const -> u16 {
        const QRect lowValue =
            handleRectForValue(lowVal).adjusted(-4, -4, 4, 4);
        const QRect highValue =
            handleRectForValue(highVal).adjusted(-4, -4, 4, 4);

        if (lowValue.contains(point)) {
            return 0;
        }

        if (highValue.contains(point)) {
            return 1;
        }

        return -1;
    }

    constexpr static u16 handleRadius = 8;

    u16 minimumValue = 0;
    u16 maximumValue = 0;
    u16 lowVal = 0;
    u16 highVal = 0;

    Dragging dragging = Dragging::None;
    u16 dragOffset = 0;
};
