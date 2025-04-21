#pragma once

#include <QLineEdit>

class CustomInput : public QLineEdit {
   public:
    using QLineEdit::QLineEdit;

   protected:
    void keyPressEvent(QKeyEvent* event) override;
};