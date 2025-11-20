#pragma once

#include "Aliases.hpp"

static thread_local u32 seed =
    duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    )
        .count();

// NOLINTBEGIN: xorshift32
inline auto randint() -> u32 {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

inline auto randint_range(const u32 min, const u32 max) -> u32 {
    const u32 range = max - min + 1;
    const u32 limit = UINT32_MAX - (UINT32_MAX % range);

    u32 x;

    do {
        x = randint();
    } while (x >= limit);

    return min + (x % range);
}

// NOLINTEND
