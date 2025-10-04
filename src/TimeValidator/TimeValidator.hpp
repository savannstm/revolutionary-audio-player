#pragma once

#include "Aliases.hpp"

#include <QString>
#include <QValidator>

class TimeValidator : public QValidator {
    Q_OBJECT

   public:
    explicit TimeValidator(QObject* parent = nullptr) : QValidator(parent) {}

    auto validate(QString& input, i32& /* pos */) const -> State override {
        if (input.isEmpty()) {
            return Intermediate;
        }

        if (input.size() > 5) {
            return Invalid;
        }

        for (i32 i = 0; i < input.size(); ++i) {
            QChar chr = input.at(i);

            if (i == 2) {
                if (chr != QChar(':')) {
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

        i32 minutes = input.mid(0, 2).toInt(&valid);
        if (!valid) {
            return Invalid;
        }

        i32 seconds = input.mid(3, 2).toInt(&valid);
        if (!valid) {
            return Invalid;
        }

        if (seconds < 0 || seconds > 59) {
            return Invalid;
        }
        if (minutes < 0 || minutes > 99) {
            return Invalid;
        }

        return Acceptable;
    }
};
