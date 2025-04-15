#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QMenu>
#include <QTimer>
#include <qevent.h>

class OptionMenu : public QMenu {
    Q_OBJECT

   public:
    explicit OptionMenu(QWidget* parent = nullptr);

   protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};
