#pragma once

#include "aliases.hpp"

#include <QDialog>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

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
        if (!this->isFullScreen()) {
            QDialog::resizeEvent(event);
            const i32 side = min(event->size().width(), event->size().height());
            resize(side, side);
        }
    }

   private:
    void showContextMenu(const QPoint& pos);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* image = new QLabel(this);
};
