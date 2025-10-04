#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QMenu>
#include <QTimer>

class OptionMenu : public QMenu {
    Q_OBJECT

   public:
    using QMenu::QMenu;

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};
