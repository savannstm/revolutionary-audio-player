#pragma once

#include "actionbutton.hpp"

class IconTextButton : public ActionButton {
    Q_OBJECT

   public:
    using ActionButton::ActionButton;

   protected:
    void paintEvent(QPaintEvent* event) override;
};