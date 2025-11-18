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
    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* const event) override {
        if (event->key() == Qt::Key_F11) {
            toggleFullscreen(isFullScreen());
        }
    }

   private:
    void showContextMenu(const QPoint& pos);
    void toggleFullscreen(bool isFullscreen);

    QPixmap originalPixmap;

    QHBoxLayout* const layout = new QHBoxLayout(this);
    QLabel* const coverLabel = new QLabel(this);
};
