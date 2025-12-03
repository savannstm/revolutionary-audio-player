#pragma once

#include <QMenu>

class OptionMenu : public QMenu {
    Q_OBJECT

   public:
    using QMenu::QMenu;

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};
