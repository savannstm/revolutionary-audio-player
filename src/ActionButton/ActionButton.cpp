#include "ActionButton.hpp"

#include <QAction>

ActionButton::ActionButton(QWidget* parent) : QPushButton(parent) {}

void ActionButton::setAction(QAction* action) {
    if ((actionOwner != nullptr) && actionOwner != action) {
        disconnect(
            this,
            &ActionButton::clicked,
            actionOwner,
            &QAction::trigger
        );
    }

    actionOwner = action;
    connect(this, &ActionButton::clicked, actionOwner, &QAction::trigger);
}
