#pragma once

#include <QValidator>

class Invalidator : public QValidator {
    Q_OBJECT

   public:
    using QValidator::QValidator;

    auto validate(QString& /* input */, int& /* pos */) const
        -> State override {
        return QValidator::Intermediate;
    }
};
