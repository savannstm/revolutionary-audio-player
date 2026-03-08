#pragma once

#include "Aliases.hpp"

#include <QString>
#include <QValidator>

class TimeValidator : public QValidator {
    Q_OBJECT

   public:
    explicit TimeValidator(QObject* const parent = nullptr) :
        QValidator(parent) {}

    auto validate(QString& input, i32& /* pos */) const -> State override {
        if (input.isEmpty()) {
            return Intermediate;
        }

        if (input.size() > 5) {
            return Invalid;
        }

        for (const i32 idx : range<i32>(0, input.size())) {
            const QChar chr = input.at(idx);

            if (idx == 2) {
                if (chr != ':') {
                    return Invalid;
                }
            } else {
                if (!chr.isDigit()) {
                    return Invalid;
                }
            }
        }

        if (input.size() < 5) {
            return Intermediate;
        }

        bool valid = false;

        const i32 minutes = input.mid(0, 2).toInt(&valid);
        if (!valid) {
            return Invalid;
        }

        const i32 seconds = input.mid(3, 2).toInt(&valid);
        if (!valid) {
            return Invalid;
        }

        if (seconds < 0 || seconds > 59) {
            return Invalid;
        }

        if (minutes < 0) {
            return Invalid;
        }

        return Acceptable;
    }
};
