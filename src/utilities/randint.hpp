#pragma once

#include "aliases.hpp"

// NOLINTBEGIN:
// these aren't magic numbers, just xor-shift prng
auto randint(u32 seed) -> u32 {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// NOLINTEND


auto randint_range(const u32 min, const u32 max, const u32 seed) -> u32 { // NOLINT: not so easily swappable
    if (min == max) {
        return min;
    }

    const u32 range = max - min + 1;
    return min + (randint(seed) % range);
}