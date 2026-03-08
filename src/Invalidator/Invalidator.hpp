#pragma once

#include "Aliases.hpp"

#include <QValidator>

class Invalidator : public QValidator {
    Q_OBJECT

   public:
    using QValidator::QValidator;

    auto validate(QString& /* input */, i32& /* pos */) const
        -> State override {
        return QValidator::Intermediate;
    }
};
