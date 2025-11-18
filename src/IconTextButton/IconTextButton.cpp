#include "IconTextButton.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionButton>

void IconTextButton::paintEvent(QPaintEvent* const event) {
    event->ignore();

    QPainter painter = QPainter(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    style()->drawControl(QStyle::CE_PushButtonBevel, &option, &painter, this);

    const QPixmap iconPixmap = icon().pixmap(iconSize());
    const QPoint iconPos = QPoint(
        (width() - iconPixmap.width()) / 2,
        (height() - iconPixmap.height()) / 2
    );
    painter.drawPixmap(iconPos, iconPixmap);

    const QRect textRect = QRect(0, 0, width(), height());
    painter.setPen(palette().buttonText().color());
    painter.drawText(textRect, Qt::AlignCenter, text());
}
