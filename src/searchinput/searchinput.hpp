#pragma once

#include <QLineEdit>

class SearchInput : public QLineEdit {
   public:
    using QLineEdit::QLineEdit;

   protected:
    void keyPressEvent(QKeyEvent* event) override;
};