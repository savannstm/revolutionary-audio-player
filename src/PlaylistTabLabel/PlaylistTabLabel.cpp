#include "PlaylistTabLabel.hpp"

#include <QMouseEvent>

PlaylistTabLabel::PlaylistTabLabel(const QString& text, QWidget* parent) :
    QLineEdit(text, parent) {
    setReadOnly(true);
    setFrame(false);
    setStyleSheet("background: transparent;");
}

void PlaylistTabLabel::mouseDoubleClickEvent(QMouseEvent* /* event */) {
    setReadOnly(false);
    setStyleSheet(QString());
    setFocus();
    selectAll();
}
