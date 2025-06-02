#include "aliases.hpp"
#include "constants.hpp"

// TODO: Existing presets need to be adjusted, and 18- with 30-band versions
// added

/// 10 Bands

// account for empty preset
constexpr u8 DEFAULT_PRESET_COUNT = 25;

// Bass-focused genres
constexpr array<i8, TEN_BANDS> PRESET_10_EMPTY = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
constexpr array<i8, TEN_BANDS> PRESET_10_HIP_HOP = { 8,  7,  5, 0, -2,
                                                     -2, -2, 0, 2, 3 };
constexpr array<i8, TEN_BANDS> PRESET_10_RNB = {
    6, 6, 4, 2, 0, -1, 0, 1, 3, 4
};
constexpr array<i8, TEN_BANDS> PRESET_10_DEEP = { 10, 9,  7, 3, 0,
                                                  -3, -2, 0, 1, 2 };

// Vocal/Clarity-focused
constexpr array<i8, TEN_BANDS> PRESET_10_SPEECH = { -3, -2, 0, 2, 4,
                                                    6,  5,  3, 0, -2 };
constexpr array<i8, TEN_BANDS> PRESET_10_VOCAL = { -2, -1, 1, 3, 5,
                                                   7,  6,  4, 2, 0 };

// Instrument-focused
constexpr array<i8, TEN_BANDS> PRESET_10_PIANO = {
    2, 3, 4, 4, 3, 2, 3, 4, 5, 4
};
constexpr array<i8, TEN_BANDS> PRESET_10_JAZZ = {
    4, 3, 2, 1, 0, 1, 2, 3, 4, 5
};
constexpr array<i8, TEN_BANDS> PRESET_10_CLASSIC = { 4, 3, 2, 1, 0,
                                                     0, 1, 2, 3, 4 };

// Energy-focused
constexpr array<i8, TEN_BANDS> PRESET_10_ROCK = { -2, 0,  2,  4, 6,
                                                  8,  10, 12, 8, 5 };
constexpr array<i8, TEN_BANDS> PRESET_10_METAL = { -3, -1, 3,  6,  8,
                                                   10, 12, 14, 10, 7 };
constexpr array<i8, TEN_BANDS> PRESET_10_ALTERNATIVE = { 2, 1, 0, -1, 0,
                                                         2, 4, 6, 4,  3 };

// High-energy electronic
constexpr array<i8, TEN_BANDS> PRESET_10_TECHNO = { 12, 10, 5, 2,  0,
                                                    2,  5,  8, 10, 12 };
constexpr array<i8, TEN_BANDS> PRESET_10_DRUM_N_BASS = { 14, 12, 6, 2,  -2,
                                                         0,  4,  8, 10, 8 };
constexpr array<i8, TEN_BANDS> PRESET_10_ELECTRONIC = { 10, 8, 5, 3,  1,
                                                        3,  6, 9, 12, 10 };

// Specialized profiles
constexpr array<i8, TEN_BANDS> PRESET_10_BASS_REDUCER = { -10, -8, -6, 0, 0,
                                                          0,   0,  0,  0, 0 };
constexpr array<i8, TEN_BANDS> PRESET_10_BASS_BOOSTER = {
    15, 12, 10, 5, 0, -5, -8, -10, -12, -14
};
constexpr array<i8, TEN_BANDS> PRESET_10_FULL_TREBLE = {
    -20, -20, -15, -10, 0, 5, 10, 15, 18, 20
};
constexpr array<i8, TEN_BANDS> PRESET_10_FULL_BASS = { 20, 18,  15,  10,  5,
                                                       0,  -15, -20, -20, -20 };
constexpr array<i8, TEN_BANDS> PRESET_10_LOUDNESS = { 12, 10, 6, 3,  0,
                                                      2,  5,  8, 10, 12 };

// Genre-specific variations
constexpr array<i8, TEN_BANDS> PRESET_10_POP = { 6, 5, 3, 1, 0, 2, 4, 6, 8, 7 };
constexpr array<i8, TEN_BANDS> PRESET_10_INDIE = {
    3, 2, 1, 0, 1, 3, 5, 7, 5, 4
};
constexpr array<i8, TEN_BANDS> PRESET_10_REGGAE = {
    8, 7, 5, 2, 0, 2, 4, 6, 8, 7
};
constexpr array<i8, TEN_BANDS> PRESET_10_SKA = { 6, 5, 3, 1, 0, 2, 5, 7, 9, 8 };
constexpr array<i8, TEN_BANDS> PRESET_10_FULL_BASS_TREBLE = {
    15, 12, 8, 3, 0, 3, 8, 12, 15, 18
};

constexpr array<array<i8, TEN_BANDS>, DEFAULT_PRESET_COUNT> PRESETS_10 = {
    PRESET_10_EMPTY,
    PRESET_10_HIP_HOP,
    PRESET_10_RNB,
    PRESET_10_DEEP,
    PRESET_10_SPEECH,
    PRESET_10_VOCAL,
    PRESET_10_PIANO,
    PRESET_10_JAZZ,
    PRESET_10_CLASSIC,
    PRESET_10_ROCK,
    PRESET_10_METAL,
    PRESET_10_ALTERNATIVE,
    PRESET_10_TECHNO,
    PRESET_10_DRUM_N_BASS,
    PRESET_10_ELECTRONIC,
    PRESET_10_BASS_REDUCER,
    PRESET_10_BASS_BOOSTER,
    PRESET_10_FULL_TREBLE,
    PRESET_10_FULL_BASS,
    PRESET_10_LOUDNESS,
    PRESET_10_POP,
    PRESET_10_INDIE,
    PRESET_10_REGGAE,
    PRESET_10_SKA,
    PRESET_10_FULL_BASS_TREBLE,
};
