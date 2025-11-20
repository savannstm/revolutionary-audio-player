#include "InputPopup.hpp"

#include <QHideEvent>
#include <QVBoxLayout>

InputPopup::InputPopup(QWidget* const parent) :
    QFrame(parent, Qt::Popup),
    lineEdit(new QLineEdit(this)) {
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFrameShape(QFrame::Box);

    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(lineEdit);

    connect(lineEdit, &QLineEdit::editingFinished, this, &InputPopup::close);
    connect(lineEdit, &QLineEdit::inputRejected, this, &InputPopup::close);
}

InputPopup::InputPopup(const QPoint& pos, QWidget* const parent) :
    InputPopup(parent) {
    move(pos);
}

InputPopup::InputPopup(
    const QString& text,
    const QPoint& pos,
    QWidget* const parent
) :
    InputPopup(parent) {
    move(pos);
    lineEdit->setText(text);
}

void InputPopup::hideEvent(QHideEvent* const event) {
    QFrame::hideEvent(event);
    close();
}

void InputPopup::closeEvent(QCloseEvent* const event) {
    emit finished(lineEdit->text());
    QFrame::closeEvent(event);
}