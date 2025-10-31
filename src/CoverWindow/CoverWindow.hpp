#pragma once

#include "Aliases.hpp"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>

class CoverWindow : public QDialog {
    Q_OBJECT

   public:
    explicit CoverWindow(
        const QString& coverPath,
        const QString& title,
        QWidget* parent = nullptr
    );

    void updateCover(const QString& coverPath);

   protected:
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);

        if (width() < originalPixmap.width()) {
            resize(originalPixmap.width(), height());
        }

        const u16 newHeight = height();

        const f64 aspectRatio =
            f64(originalPixmap.width()) / originalPixmap.height();
        const u16 newWidth = u16(newHeight * aspectRatio);

        const QPixmap scaledPixmap = originalPixmap.scaled(
            newWidth,
            newHeight,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        coverLabel->setPixmap(scaledPixmap);
        coverLabel->resize(scaledPixmap.size());
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_F11) {
            toggleFullscreen(isFullScreen());
        }
    }

   private:
    void showContextMenu(const QPoint& pos);
    void toggleFullscreen(bool isFullscreen);

    QHBoxLayout* layout = new QHBoxLayout(this);

    QPixmap originalPixmap;
    QLabel* coverLabel = new QLabel(this);
};
