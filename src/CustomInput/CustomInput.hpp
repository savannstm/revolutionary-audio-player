#pragma once

#include <QLineEdit>

class CustomInput : public QLineEdit {
    Q_OBJECT

   public:
    using QLineEdit::QLineEdit;

   signals:
    void unfocused();

   protected:
    void keyPressEvent(QKeyEvent* event) override;
};