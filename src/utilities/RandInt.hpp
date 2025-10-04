#pragma once

#include "Aliases.hpp"

// NOLINTBEGIN: these aren't magic numbers, just xor-shift prng
inline auto randint(u32 seed) -> u32 {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// NOLINTEND

inline auto randint_range(
    const u32 min,
    const u32 max,  // NOLINT: not so easily swappable
    const u32 seed
) -> u32 {
    if (min == max) {
        return min;
    }

    const u32 range = max - min + 1;
    return min + (randint(seed) % range);
}