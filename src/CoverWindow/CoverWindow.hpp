#pragma once

#include "Aliases.hpp"
#include "FWD.hpp"

#include <QDialog>

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
    void keyPressEvent(QKeyEvent* event) override;

   private:
    inline void showContextMenu(const QPoint& pos);
    inline void toggleFullscreen(bool isFullscreen);

    QPixmap originalPixmap;

    QHBoxLayout* const layout;
    QLabel* const coverLabel;
};
