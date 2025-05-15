#include "aliases.hpp"

#include <QIntValidator>

class IntValidator : public QIntValidator {
   public:
    using QIntValidator::QIntValidator;

    void fixup(QString& input) const override {
        bool valid;
        i32 value = input.toInt(&valid);

        if (valid) {
            if (value < bottom()) {
                input = QString::number(bottom());
            } else if (value > top()) {
                input = QString::number(top());
            }
        }
    }
};