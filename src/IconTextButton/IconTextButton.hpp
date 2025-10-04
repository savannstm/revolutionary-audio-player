#pragma once

#include "ActionButton.hpp"

#include <QMenu>
#include <QMouseEvent>

class IconTextButton : public ActionButton {
    Q_OBJECT

   public:
    using ActionButton::ActionButton;

   protected:
    void paintEvent(QPaintEvent* event) override;
};