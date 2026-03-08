#pragma once

#include <QPushButton>

class ActionButton : public QPushButton {
    Q_OBJECT

   public:
    using QPushButton::QPushButton;
    void setAction(QAction* action);

   private:
    QAction* actionOwner = nullptr;
};