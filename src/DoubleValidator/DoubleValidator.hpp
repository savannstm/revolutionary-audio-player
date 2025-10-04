#include "Aliases.hpp"

#include <QDoubleValidator>

class DoubleValidator : public QDoubleValidator {
   public:
    using QDoubleValidator::QDoubleValidator;

    void fixup(QString& input) const override {
        if (input.isEmpty()) {
            input = QString::number(emptyFallback, 'g', decimals());
            return;
        }

        bool valid;
        f64 value = input.toDouble(&valid);

        if (valid) {
            if (value < bottom()) {
                input = QString::number(bottom(), 'g', decimals());
            } else if (value > top()) {
                input = QString::number(top(), 'g', decimals());
            }
        }
    }

    f32 emptyFallback;
};