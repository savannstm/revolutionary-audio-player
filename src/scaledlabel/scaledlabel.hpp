#pragma once

#include <QLabel>
#include <QPixmap>

class ScaledLabel : public QLabel {
    QPixmap originalPixmap;

   public:
    ScaledLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setAlignment(Qt::AlignCenter);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    void setPixmap(const QPixmap& pixmap) {
        originalPixmap = pixmap;
        updateScaledPixmap();
    }

   protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        updateScaledPixmap();
    }

   private:
    void updateScaledPixmap() {
        if (!originalPixmap.isNull()) {
            QPixmap scaled = originalPixmap.scaled(
                size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            QLabel::setPixmap(scaled);
        }
    }
};
