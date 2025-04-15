#pragma once

#include "aliases.hpp"

inline auto toMinutes(const u16 secs) -> string {
    const u8 minutes = secs / 60;
    const u8 seconds = secs % 60;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = format("{}:{}", minutes, secondsPadded);
    return formatted;
}
