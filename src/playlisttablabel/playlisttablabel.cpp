#include "playlisttablabel.hpp"

#include <QMouseEvent>

PlaylistTabLabel::PlaylistTabLabel(const QString& text, QWidget* parent) :
    QLineEdit(text, parent) {
    setText(text);
    setReadOnly(true);
    setFrame(false);
    setStyleSheet("background: transparent;");
}

void PlaylistTabLabel::mouseDoubleClickEvent(QMouseEvent* event) {
    setReadOnly(false);
    setStyleSheet("");
    setFocus();
    selectAll();
    emit doubleClicked();
}

void PlaylistTabLabel::mousePressEvent(QMouseEvent* event) {
    const auto mouseButton = event->button();

    switch (mouseButton) {
        case Qt::MouseButton::LeftButton:
            emit clicked();
            break;
        default:
            break;
    }
}

void PlaylistTabLabel::contextMenuEvent(QContextMenuEvent* event) {
    event->ignore();
    emit rightClicked();
}