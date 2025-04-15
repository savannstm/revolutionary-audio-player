#include "optionmenu.hpp"

#include <QMouseEvent>

OptionMenu::OptionMenu(QWidget* parent) : QMenu(parent) {}

void OptionMenu::mouseReleaseEvent(QMouseEvent* event) {
    QAction* action = activeAction();

    if (action != nullptr) {
        action->trigger();
    } else {
        QMenu::mouseReleaseEvent(event);
    }
}