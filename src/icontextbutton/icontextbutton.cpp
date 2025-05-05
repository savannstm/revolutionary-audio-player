#include "icontextbutton.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionButton>

void IconTextButton::paintEvent(QPaintEvent* event) {
    event->ignore();

    QPainter painter(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    style()->drawControl(QStyle::CE_PushButtonBevel, &option, &painter, this);

    QPixmap iconPixmap = icon().pixmap(iconSize());
    QPoint iconPos(
        (width() - iconPixmap.width()) / 2,
        (height() - iconPixmap.height()) / 2
    );
    painter.drawPixmap(iconPos, iconPixmap);

    QRect textRect = QRect(0, 0, width(), height());
    painter.setPen(palette().buttonText().color());
    painter.drawText(textRect, Qt::AlignCenter, text());
}
