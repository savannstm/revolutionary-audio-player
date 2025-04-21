#include "custominput.hpp"

#include <QKeyEvent>

void CustomInput::keyPressEvent(QKeyEvent* event) {
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