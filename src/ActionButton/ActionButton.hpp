#pragma once

#include <QAction>
#include <QPushButton>

class ActionButton : public QPushButton {
    Q_OBJECT

   public:
    explicit ActionButton(QWidget* parent = nullptr);
    void setAction(QAction* action);

   private:
    QAction* actionOwner = nullptr;
};