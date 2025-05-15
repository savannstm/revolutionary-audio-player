#pragma once

#include <QObject>
#include <QValidator>

class Invalidator : public QValidator {
    Q_OBJECT

   public:
    using QValidator::QValidator;

    auto validate(QString& input, int& pos) const -> State override {
        Q_UNUSED(input);
        Q_UNUSED(pos);
        return QValidator::Intermediate;
    }
};
