#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QMenu>
#include <QTimer>
#include <qevent.h>

class OptionMenu : public QMenu {
    Q_OBJECT

   public:
    using QMenu::QMenu;

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};
