#pragma once

#include "Aliases.hpp"

#include <QWidget>

class RangeSlider : public QWidget {
    Q_OBJECT

   public:
    explicit RangeSlider(QWidget* parent = nullptr);

    [[nodiscard]] auto minimum() const -> i32 { return minimumValue; }

    [[nodiscard]] auto maximum() const -> i32 { return maximumValue; }

    [[nodiscard]] auto lowValue() const -> i32 { return lowVal; }

    [[nodiscard]] auto highValue() const -> i32 { return highVal; }

    void setMinimum(i32 minimum);
    void setMaximum(i32 maximum);

    void setRange(i32 minVal, i32 maxVal);

    void setLowValue(i32 value);
    void setHighValue(i32 value);

   signals:
    void minimumChanged(i32 value);
    void maximumChanged(i32 value);
    void lowValueChanged(i32 value);
    void highValueChanged(i32 value);
    void rangeChanged(i32 low, i32 high);

   protected:
    [[nodiscard]] auto minimumSizeHint() const -> QSize override {
        return { MINIMUM_WIDTH, MINIMUM_HEIGHT };
    }

    void paintEvent(QPaintEvent* /* event */) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* const /* event */) override {
        dragging = None;
    }

    void keyPressEvent(QKeyEvent* event) override;

   private:
    enum Dragging : u8 {
        None = 0,
        Low = 1,
        High = 2
    };

    [[nodiscard]] auto grooveRect() const -> QRect;
    [[nodiscard]] auto handleRectForValue(i32 value) const -> QRect;
    [[nodiscard]] auto selectionRect() const -> QRect;
    [[nodiscard]] auto valueToPos(i32 value) const -> i32;
    [[nodiscard]] auto posToPoint(i32 value) const -> QPoint;
    [[nodiscard]] auto posToValue(i32 pos) const -> i32;
    [[nodiscard]] auto hitTest(const QPoint& point) const -> i32;

    static constexpr u16 DEFAULT_MAX = 100;
    static constexpr u16 MINIMUM_WIDTH = 128;
    static constexpr u16 MINIMUM_HEIGHT = 48;
    static constexpr u16 HANDLE_RADIUS = 8;

    i32 minimumValue = 0;
    i32 maximumValue = 0;
    i32 lowVal = 0;
    i32 highVal = 0;

    Dragging dragging = Dragging::None;
    i32 dragOffset = 0;
};
