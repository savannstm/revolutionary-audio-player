#include "Duration.hpp"

#include "Constants.hpp"

#include <QChar>

Duration::Duration() = default;

Duration::Duration(const u32 totalSeconds) : totalSecs(totalSeconds) {}

Duration::Duration(const QStringView string) {
    if (const auto dur = fromString(string)) {
        totalSecs = dur->totalSecs;
    } else {
        totalSecs = 0;
    }
}

auto Duration::fromString(const QStringView string) -> optional<Duration> {
    const u8 colonCount = string.count(':');
    QLatin1StringView deduced;

    switch (colonCount) {
        case 0:
            deduced = "s"_L1;
            break;
        case 1:
            deduced = "m:s"_L1;
            break;
        case 2:
            deduced = "h:m:s"_L1;
            break;
        default:
            return nullopt;
    }

    return tryParse(string, deduced);
}

auto Duration::stringToSeconds(const QStringView string) -> optional<u32> {
    if (const auto dur = fromString(string)) {
        return dur->toSeconds();
    }

    return nullopt;
}

auto Duration::toString(const QLatin1StringView format) const -> QString {
    QString result;

    const bool hasH = format.contains('h');
    const bool hasM = format.contains('m');
    const bool hasS = format.contains('s');

    u32 hrs = 0;
    u32 mins = 0;
    u32 secs = 0;

    if (!hasH && hasM && hasS) {
        mins = totalSecs / MINUTE_SECONDS;
        secs = totalSecs % MINUTE_SECONDS;
    } else if (!hasH && !hasM && hasS) {
        secs = totalSecs;
    } else {
        hrs = hours();
        mins = minutes();
        secs = seconds();
    }

    for (u32 i = 0; i < format.size();) {
        const QChar chr = format[i];

        if (chr == 'h' || chr == 'm' || chr == 's') {
            u32 count = 1;
            while (i + count < format.size() && format[i + count] == chr) {
                count++;
            }

            const u32 value = (chr == 'h') ? hrs : (chr == 'm') ? mins : secs;

            QString part = QString::number(value);
            if (count > 1) {
                part = part.rightJustified(count, '0');
            }

            result += part;
            i += count;
        } else {
            result += chr;
            i++;
        }
    }

    return result;
}

auto Duration::secondsToString(
    const u32 totalSeconds,
    const QLatin1StringView format
) -> QString {
    return Duration(totalSeconds).toString(format);
}

auto Duration::hours() const -> u32 {
    return totalSecs / HOUR_SECONDS;
}

auto Duration::minutes() const -> u32 {
    return (totalSecs / MINUTE_SECONDS) % MINUTE_SECONDS;
}

auto Duration::seconds() const -> u32 {
    return totalSecs % MINUTE_SECONDS;
}

auto Duration::toSeconds() const -> u32 {
    return totalSecs;
}

auto Duration::parse(const QStringView string, u32& pos, u32& value) -> bool {
    while (pos < string.size() && string[pos] == '0') {
        pos++;
    }

    const u32 start = pos;
    u32 digits = 0;

    while (pos < string.size() && (std::isdigit(string[pos].toLatin1()) != 0)) {
        pos++;
        digits++;
    }

    if (start == pos && string.size() > 0 && pos > 0 &&
        string[pos - 1] == '0') {
        value = 0;
        return true;
    }

    if (digits == 0 && start == pos) {
        return false;
    }

    if (digits == 0) {
        value = 0;
        return true;
    }

    value = string.mid(start, digits).toInt();
    return true;
}

auto Duration::tryParse(
    const QStringView string,
    const QLatin1StringView format
) -> std::optional<Duration> {
    u32 posText = 0;
    u32 posFmt = 0;

    u32 hrs = 0;
    u32 mins = 0;
    u32 secs = 0;

    bool success = true;

    while ((posFmt < format.size()) && success) {
        const QChar chr = format[posFmt];

        if (chr == 'h' || chr == 'm' || chr == 's') {
            u32 count = 1;

            while (((posFmt + count) < format.size()) &&
                   format[posFmt + count] == chr) {
                count++;
            }

            u32 value = 0;

            if (!parse(string, posText, value)) {
                success = false;
                break;
            }

            if (chr == 'h') {
                hrs = value;
            }

            if (chr == 'm') {
                mins = value;
            }

            if (chr == 's') {
                secs = value;
            }

            posFmt += count;
        } else {
            if (posText >= string.size() || string[posText] != chr) {
                success = false;
                break;
            }

            posFmt++;
            posText++;
        }
    }

    if (posText != string.size()) {
        success = false;
    }

    if (!success) {
        return nullopt;
    }

    const u32 total = (hrs * HOUR_SECONDS) + (mins * MINUTE_SECONDS) + secs;
    return Duration(total);
}

auto Duration::operator+(const Duration rhs) const -> Duration {
    return Duration(this->totalSecs + rhs.totalSecs);
}

auto Duration::operator+(const u32 rhs) const -> Duration {
    return Duration(this->totalSecs + rhs);
}

auto Duration::operator+=(const Duration rhs) -> Duration& {
    this->totalSecs += rhs.totalSecs;
    return *this;
}

auto Duration::operator+=(const u32 rhs) -> Duration& {
    this->totalSecs += rhs;
    return *this;
}

auto Duration::operator-=(const Duration rhs) -> Duration& {
    this->totalSecs -= rhs.totalSecs;
    return *this;
}

auto Duration::operator-=(const u32 rhs) -> Duration& {
    this->totalSecs -= rhs;
    return *this;
}