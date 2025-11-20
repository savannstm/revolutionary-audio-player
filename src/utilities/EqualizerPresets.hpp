#include "Aliases.hpp"
#include "Enums.hpp"

//! All of these presets were made by ChatGPT.
// and I actually like them

/// 10 Bands

// account for empty preset
constexpr u8 DEFAULT_PRESET_COUNT = 25;

// Bass-focused genres
constexpr array<i8, TEN_BANDS> PRESET_10_EMPTY{};
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

/// 18 Bands

// Bass-focused genres
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_EMPTY{};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_HIP_HOP = {
    8, 8, 7, 6, 4, 2, 0, -2, -2, -2, -2, -1, 0, 1, 2, 2, 3, 3
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_RNB = { 6, 6, 5,  4, 3, 2,
                                                      1, 0, -1, 0, 0, 1,
                                                      2, 3, 3,  4, 4, 4 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_DEEP = { 10, 10, 9,  8,  6,  4,
                                                       2,  0,  -2, -3, -2, -1,
                                                       0,  0,  1,  1,  2,  2 };

// Vocal/Clarity-focused
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_SPEECH = {
    -3, -3, -2, -1, 0, 1, 3, 4, 5, 6, 6, 5, 4, 2, 1, 0, -1, -2
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_VOCAL = { -2, -2, -1, 0, 1, 2,
                                                        4,  5,  6,  7, 7, 6,
                                                        5,  3,  2,  1, 1, 0 };

// Instrument-focused
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_PIANO = { 2, 2, 3, 3, 4, 4,
                                                        4, 3, 3, 2, 2, 3,
                                                        3, 4, 4, 4, 5, 4 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_JAZZ = { 4, 4, 3, 3, 2, 2,
                                                       1, 1, 0, 0, 1, 1,
                                                       2, 3, 3, 4, 4, 5 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_CLASSIC = { 4, 4, 3, 3, 2, 2,
                                                          1, 1, 0, 0, 0, 1,
                                                          1, 2, 3, 3, 4, 4 };

// Energy-focused
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_ROCK = { -2, -2, 0, 1,  2,  4,
                                                       5,  7,  8, 10, 11, 11,
                                                       12, 10, 9, 7,  6,  5 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_METAL = { -3, -2, 0,  2,  3,  5,
                                                        7,  8,  10, 11, 12, 13,
                                                        14, 12, 11, 9,  8,  7 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_ALTERNATIVE = {
    2, 2, 1, 1, 0, -1, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 3, 3
};

// High-energy electronic
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_TECHNO = {
    12, 12, 10, 8, 6, 5, 3, 2, 0, 1, 3, 5, 7, 8, 9, 10, 11, 12
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_DRUM_N_BASS = {
    14, 13, 12, 10, 8, 6, 3, 1, -1, 0, 2, 4, 6, 8, 9, 10, 9, 8
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_ELECTRONIC = {
    10, 10, 8, 6, 5, 4, 3, 2, 1, 2, 4, 6, 8, 9, 10, 11, 11, 10
};

// Specialized profiles
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_BASS_REDUCER = {
    -10, -10, -8, -7, -6, -4, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_BASS_BOOSTER = {
    15, 14, 12, 11, 10, 8, 5, 2, 0, -3, -5, -7, -9, -10, -11, -12, -13, -14
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_FULL_TREBLE = {
    -20, -20, -18, -16, -13, -10, -5, 0, 3, 6, 9, 12, 15, 17, 18, 19, 19, 20
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_FULL_BASS = {
    20, 19, 18, 16, 14, 12, 9, 6, 3, 0, -5, -10, -15, -18, -19, -20, -20, -20
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_LOUDNESS = {
    12, 12, 10, 8, 6, 4, 3, 1, 0, 1, 3, 5, 7, 9, 10, 11, 11, 12
};

// Genre-specific variations
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_POP = {
    6, 6, 5, 4, 3, 2, 1, 0, 0, 1, 3, 4, 6, 7, 8, 8, 8, 7
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_INDIE = { 3, 3, 2, 2, 1, 1,
                                                        0, 1, 2, 3, 4, 5,
                                                        6, 6, 5, 5, 4, 4 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_REGGAE = { 8, 8, 7, 6, 5, 3,
                                                         2, 1, 0, 1, 3, 5,
                                                         6, 7, 8, 8, 8, 7 };
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_SKA = {
    6, 6, 5, 4, 3, 2, 1, 0, 0, 1, 3, 5, 6, 7, 8, 9, 9, 8
};
constexpr array<i8, EIGHTEEN_BANDS> PRESET_18_FULL_BASS_TREBLE = {
    15, 14, 12, 10, 8, 5, 3, 1, 0, 1, 3, 6, 8, 10, 12, 14, 16, 18
};

constexpr array<array<i8, EIGHTEEN_BANDS>, DEFAULT_PRESET_COUNT> PRESETS_18 = {
    PRESET_18_EMPTY,
    PRESET_18_HIP_HOP,
    PRESET_18_RNB,
    PRESET_18_DEEP,
    PRESET_18_SPEECH,
    PRESET_18_VOCAL,
    PRESET_18_PIANO,
    PRESET_18_JAZZ,
    PRESET_18_CLASSIC,
    PRESET_18_ROCK,
    PRESET_18_METAL,
    PRESET_18_ALTERNATIVE,
    PRESET_18_TECHNO,
    PRESET_18_DRUM_N_BASS,
    PRESET_18_ELECTRONIC,
    PRESET_18_BASS_REDUCER,
    PRESET_18_BASS_BOOSTER,
    PRESET_18_FULL_TREBLE,
    PRESET_18_FULL_BASS,
    PRESET_18_LOUDNESS,
    PRESET_18_POP,
    PRESET_18_INDIE,
    PRESET_18_REGGAE,
    PRESET_18_SKA,
    PRESET_18_FULL_BASS_TREBLE,
};

/// 30 Bands

// Bass-focused genres
constexpr array<i8, THIRTY_BANDS> PRESET_30_EMPTY{};
constexpr array<i8, THIRTY_BANDS> PRESET_30_HIP_HOP = {
    8,  8,  8,  7,  7,  6, 5, 4, 3, 2, 1, 0, -1, -2, -2,
    -2, -2, -2, -2, -1, 0, 0, 1, 2, 2, 3, 3, 3,  3,  3
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_RNB = { 6, 6, 6, 6, 5,  5, 4, 4,
                                                    3, 2, 1, 0, -1, 0, 0, 0,
                                                    0, 0, 0, 1, 2,  2, 3, 3,
                                                    4, 4, 4, 4, 4,  4 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_DEEP = {
    10, 10, 10, 9,  9,  8, 7, 6, 5, 3, 2, 0, -1, -2, -3,
    -3, -3, -2, -2, -1, 0, 0, 0, 0, 1, 1, 2, 2,  2,  2
};

// Vocal / Clarity-focused
constexpr array<i8, THIRTY_BANDS> PRESET_30_SPEECH = {
    -3, -3, -3, -2, -2, -1, 0, 1,  2,  3,  4,  5,  6,  6,  6,
    5,  4,  3,  2,  1,  0,  0, -1, -1, -1, -1, -1, -1, -2, -2
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_VOCAL = {
    -2, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 7,
    6,  6,  5,  4,  3,  2, 2, 1, 1, 1, 1, 1, 1, 0, 0
};

// Instrument-focused
constexpr array<i8, THIRTY_BANDS> PRESET_30_PIANO = { 2, 2, 2, 3, 3, 3, 4, 4,
                                                      4, 4, 3, 3, 3, 3, 2, 2,
                                                      2, 2, 2, 3, 3, 3, 4, 4,
                                                      4, 4, 4, 4, 5, 4 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_JAZZ = { 4, 4, 4, 4, 3, 3, 3, 2,
                                                     2, 2, 1, 1, 1, 0, 0, 0,
                                                     0, 1, 1, 1, 1, 2, 3, 3,
                                                     3, 4, 4, 4, 4, 5 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_CLASSIC = { 4, 4, 4, 3, 3, 3, 2, 2,
                                                        2, 1, 1, 1, 0, 0, 0, 0,
                                                        0, 0, 1, 1, 1, 2, 2, 3,
                                                        3, 3, 4, 4, 4, 4 };

// Energy-focused
constexpr array<i8, THIRTY_BANDS> PRESET_30_ROCK = {
    -2, -2, -2, -1, 0,  1,  2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 12, 12, 11, 10, 9, 8, 7, 6, 6, 5, 5, 5, 5
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_METAL = {
    -3, -3, -3, -2, -1, 0,  1,  2,  3,  4,  5,  6, 7, 8, 9,
    10, 11, 12, 13, 13, 14, 14, 13, 12, 11, 10, 9, 8, 8, 7
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_ALTERNATIVE = {
    2, 2, 2, 2, 1, 1, 0, 0, -1, -1, 0, 0, 1, 2, 2,
    3, 4, 4, 5, 5, 6, 6, 6, 5,  5,  4, 4, 3, 3, 3
};

// High-energy electronic
constexpr array<i8, THIRTY_BANDS> PRESET_30_TECHNO = {
    12, 12, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
    1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 11, 12, 12, 12
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_DRUM_N_BASS = {
    14, 14, 14, 13, 12, 11, 10, 9, 8, 7, 5, 3, 1, 0, -1,
    -1, 0,  1,  2,  3,  5,  6,  7, 8, 9, 9, 9, 8, 8, 8
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_ELECTRONIC = {
    10, 10, 10, 9, 8, 7, 6, 5, 4,  3,  2,  2,  1,  1,  1,
    2,  3,  4,  5, 6, 7, 8, 9, 10, 11, 11, 11, 11, 10, 10
};

// Specialized profiles
constexpr array<i8, THIRTY_BANDS> PRESET_30_BASS_REDUCER = {
    -10, -10, -10, -9, -8, -8, -7, -7, -6, -5, -4, -3, -2, -1, 0,
    0,   0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_BASS_BOOSTER = {
    15, 15, 14, 13, 12, 11, 10, 9,   8,   7,   6,   5,   3,   1,   0,
    -2, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -13, -14, -14, -14
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_FULL_TREBLE = {
    -20, -20, -20, -19, -18, -17, -16, -15, -13, -11, -9, -7, -5, -3, 0,
    2,   5,   7,   9,   11,  13,  15,  17,  18,  19,  20, 20, 20, 20, 20
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_FULL_BASS = {
    20, 20, 20, 19, 18,  17,  16,  15,  13,  11,  9,   7,   5,   3,   1,
    0,  -3, -6, -9, -12, -15, -17, -18, -19, -19, -20, -20, -20, -20, -20
};
constexpr array<i8, THIRTY_BANDS> PRESET_30_LOUDNESS = {
    12, 12, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
    1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 11, 11, 12, 12
};

// Genre-specific variations
constexpr array<i8, THIRTY_BANDS> PRESET_30_POP = { 6, 6, 6, 5, 5, 4, 3, 2,
                                                    1, 0, 0, 1, 2, 3, 4, 5,
                                                    6, 6, 7, 7, 8, 8, 8, 8,
                                                    8, 8, 8, 8, 8, 7 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_INDIE = { 3, 3, 3, 3, 2, 2, 2, 1,
                                                      1, 0, 1, 2, 3, 4, 5, 5,
                                                      6, 6, 6, 6, 6, 5, 5, 5,
                                                      5, 5, 5, 5, 4, 4 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_REGGAE = { 8, 8, 8, 7, 7, 6, 5, 4,
                                                       3, 2, 1, 0, 0, 1, 2, 3,
                                                       4, 5, 6, 6, 7, 7, 8, 8,
                                                       8, 8, 8, 8, 8, 7 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_SKA = { 6, 6, 6, 5, 5, 4, 3, 2,
                                                    1, 0, 0, 1, 2, 3, 4, 5,
                                                    6, 6, 7, 8, 8, 8, 9, 9,
                                                    9, 9, 9, 9, 9, 8 };
constexpr array<i8, THIRTY_BANDS> PRESET_30_FULL_BASS_TREBLE = {
    15, 15, 14, 13, 12, 11, 10, 9, 8, 7,  5,  4,  3,  2,  1,
    0,  1,  2,  3,  4,  5,  7,  8, 9, 10, 12, 14, 16, 17, 18
};

constexpr array<array<i8, THIRTY_BANDS>, DEFAULT_PRESET_COUNT> PRESETS_30 = {
    PRESET_30_EMPTY,
    PRESET_30_HIP_HOP,
    PRESET_30_RNB,
    PRESET_30_DEEP,
    PRESET_30_SPEECH,
    PRESET_30_VOCAL,
    PRESET_30_PIANO,
    PRESET_30_JAZZ,
    PRESET_30_CLASSIC,
    PRESET_30_ROCK,
    PRESET_30_METAL,
    PRESET_30_ALTERNATIVE,
    PRESET_30_TECHNO,
    PRESET_30_DRUM_N_BASS,
    PRESET_30_ELECTRONIC,
    PRESET_30_BASS_REDUCER,
    PRESET_30_BASS_BOOSTER,
    PRESET_30_FULL_TREBLE,
    PRESET_30_FULL_BASS,
    PRESET_30_LOUDNESS,
    PRESET_30_POP,
    PRESET_30_INDIE,
    PRESET_30_REGGAE,
    PRESET_30_SKA,
    PRESET_30_FULL_BASS_TREBLE,
};
