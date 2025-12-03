#pragma once

#include "Constants.hpp"

#include <QLabel>
#include <QPixmap>

class ScaledLabel : public QLabel {
   public:
    ScaledLabel(QWidget* const parent = nullptr) : QLabel(parent) {
        setMinimumSize(MINIMUM_COVER_SIZE);
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    void setPixmap(const QPixmap& pixmap) {
        originalPixmap = pixmap;
        updateScaledPixmap();
    }

   protected:
    void resizeEvent(QResizeEvent* const event) override {
        QLabel::resizeEvent(event);
        updateScaledPixmap();
    }

   private:
    QPixmap originalPixmap;

    void updateScaledPixmap() {
        if (!originalPixmap.isNull()) {
            const QPixmap scaled = originalPixmap.scaled(
                size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            QLabel::setPixmap(scaled);
        }
    }
};
