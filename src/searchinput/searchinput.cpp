#include "searchinput.hpp"

#include <QKeyEvent>

void SearchInput::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key::Key_Escape:
            emit inputRejected();
            break;
        case Qt::Key::Key_Enter:
            emit returnPressed();
            break;
        default:
            QLineEdit::keyPressEvent(event);
            break;
    }
}