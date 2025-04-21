#pragma once

#include "aliases.hpp"

#include <QString>

inline auto toMinutes(const u16 secs) -> QString {
    const u8 minutes = secs / 60;
    const u8 seconds = secs % 60;

    const QString secondsString = QString::number(seconds);
    const QString secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const QString formatted = QString("%1:%2").arg(minutes).arg(secondsPadded);
    return formatted;
}
