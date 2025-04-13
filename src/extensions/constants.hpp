#pragma once

#include "aliases.hpp"

constexpr u8 SAMPLE_SIZE = 2;
constexpr array<cstr, 9> EXTENSIONS = { ".mp3", ".flac", ".opus",
                                        ".aac", ".wav",  ".ogg",
                                        ".m4a", ".mp4",  ".mkv" };
constexpr u8 TIMER_UPDATE_INTERVAL_MS = 100;
constexpr u8 EQ_BANDS_N = 10;
constexpr u8 MAX_VOLUME = 100;