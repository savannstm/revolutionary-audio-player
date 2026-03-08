#include "ActionButton.hpp"

#include <QAction>

void ActionButton::setAction(QAction* const action) {
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
