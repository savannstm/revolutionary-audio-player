#include "optionmenu.hpp"

#include <QMouseEvent>

void OptionMenu::mouseReleaseEvent(QMouseEvent* event) {
    QAction* action = activeAction();

    if (action != nullptr) {
        action->trigger();
    } else {
        QMenu::mouseReleaseEvent(event);
    }
}