#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"

#include <QString>
#include <QStringList>
#include <QTextStream>

inline auto timeToSecs(const QString& time) -> u16 {
    const QStringList parts = time.split(':');
    u16 seconds;

    if (parts.size() == 3) {
        const u16 hours = parts[0].toUInt();
        const u16 minutes = parts[1].toUInt();
        const u16 secs = parts[2].toUInt();
        seconds = (hours * HOUR_SECONDS) + (minutes * MINUTE_SECONDS) + secs;
    } else if (parts.size() == 2) {
        const u16 minutes = parts[0].toUInt();
        const u16 secs = parts[1].toUInt();
        seconds = (minutes * MINUTE_SECONDS) + secs;
    }

    return seconds;
}

inline auto secsToMins(const u16 totalSeconds) -> QString {
    const u16 minutes = (totalSeconds % HOUR_SECONDS) / MINUTE_SECONDS;
    const u16 seconds = totalSeconds % MINUTE_SECONDS;

    QString result;
    result.reserve(5);

    result.append(QChar('0' + (minutes / 10)));
    result.append(QChar('0' + (minutes % 10)));

    result.append(':');

    result.append(QChar('0' + (seconds / 10)));
    result.append(QChar('0' + (seconds % 10)));

    return result;
}