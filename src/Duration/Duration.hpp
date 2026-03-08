#pragma once

#include "Aliases.hpp"

#include <QString>

class Duration {
   public:
    explicit Duration();
    explicit Duration(u32 totalSeconds);
    explicit Duration(QStringView string);

    static auto fromString(QStringView string) -> optional<Duration>;
    static auto stringToSeconds(QStringView string) -> optional<u32>;

    [[nodiscard]] auto toString(QLatin1StringView format = "mm:ss"_L1) const
        -> QString;
    static auto secondsToString(
        u32 totalSeconds,
        QLatin1StringView format = "mm:ss"_L1
    ) -> QString;

    [[nodiscard]] auto hours() const -> u32;
    [[nodiscard]] auto minutes() const -> u32;
    [[nodiscard]] auto seconds() const -> u32;

    [[nodiscard]] auto toSeconds() const -> u32;

    [[nodiscard]] auto operator+(Duration rhs) const -> Duration;
    [[nodiscard]] auto operator+(u32 rhs) const -> Duration;

    auto operator+=(Duration rhs) -> Duration&;
    auto operator+=(u32 rhs) -> Duration&;

    auto operator-=(Duration rhs) -> Duration&;
    auto operator-=(u32 rhs) -> Duration&;

   private:
    u32 totalSecs = 0;

    static auto parse(QStringView string, u32& pos, u32& value) -> bool;

    static auto tryParse(QStringView string, QLatin1StringView format)
        -> optional<Duration>;
};