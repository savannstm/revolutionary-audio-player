#include "OptionMenu.hpp"

#include <QMouseEvent>

void OptionMenu::mouseReleaseEvent(QMouseEvent* const event) {
    QAction* const action = activeAction();

    if (action != nullptr) {
        action->trigger();
    } else {
        QMenu::mouseReleaseEvent(event);
    }
}