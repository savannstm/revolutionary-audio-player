#include "PlaylistTabLabel.hpp"

#include <QMouseEvent>

using namespace Qt::Literals::StringLiterals;

PlaylistTabLabel::PlaylistTabLabel(const QString& text, QWidget* const parent) :
    QLineEdit(text, parent) {
    setReadOnly(true);
    setFrame(false);
    setStyleSheet(u"background: transparent;"_s);
}

void PlaylistTabLabel::mouseDoubleClickEvent(QMouseEvent* const /* event */) {
    setReadOnly(false);
    setStyleSheet(QString());
    setFocus();
    selectAll();
}
