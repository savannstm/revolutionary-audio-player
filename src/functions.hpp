#include "type_aliases.hpp"

auto inline formatSecondsToMinutes(const u64 secs) -> string {
    const u64 minutes = secs / 60;
    const u64 seconds = secs % 60;

    const string secondsString = to_string(seconds);
    const string secondsPadded =
        seconds < 10 ? "0" + secondsString : secondsString;

    const string formatted = format("{}:{}", minutes, secondsPadded);
    return formatted;
}