#include "CustomInput.hpp"

#include <QKeyEvent>

void CustomInput::keyPressEvent(QKeyEvent* const event) {
    if (event->key() == Qt::Key::Key_Escape ||
        event->key() == Qt::Key::Key_Enter) {
        emit unfocused();
    }

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