#pragma once

#include "Aliases.hpp"

#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>

auto applyEffectToImage(
    const QImage& src,
    QGraphicsEffect* const effect,
    i32 extent = 0
) -> QImage {
    if (src.isNull()) {
        return {};
    }

    if (effect == nullptr) {
        return src;
    }

    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(src));
    item.setGraphicsEffect(effect);
    scene.addItem(&item);
    QImage res(
        src.size() + QSize(extent * 2, extent * 2),
        QImage::Format_ARGB32
    );
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(
        &ptr,
        QRectF(),
        QRectF(
            -extent,
            -extent,
            src.width() + (extent * 2),
            src.height() + (extent * 2)
        )
    );
    return res;
}