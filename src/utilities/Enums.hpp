#pragma once

#include "Aliases.hpp"

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

#define ENUM_CONSTANT(EnumType, Member, NAME, SUFFIX) \
    constexpr auto NAME##_##SUFFIX = \
        static_cast<std::underlying_type_t<EnumType>>(EnumType::Member)

enum class TrackProperty : u8 {
    Play,
    Title,
    Artist,
    TrackNumber,
    Album,
    AlbumArtist,
    Genre,
    Year,
    Duration,
    Composer,
    BPM,
    Language,
    DiscNumber,
    Comment,
    Publisher,
    Bitrate,
    SampleRate,
    Channels,
    Format,
    Path,
    Order
};

constexpr u8 TRACK_PROPERTY_COUNT = magic_enum::enum_count<TrackProperty>();

enum class Direction : u8 {
    Forward,
    BackwardRandom,
    Backward,
    ForwardRandom
};

enum class RepeatMode : u8 {
    Off,
    Track,
    Playlist,
};

enum class PlaylistNaming : u8 {
    DirectoryName,
    TrackMetadata,
    Numbered
};

enum class PlaylistFileType : u8 {
    XSPF,
    M3U,
    CUE
};

enum class TabRemoveMode : u8 {
    Single,
    ToLeft,
    ToRight,
    Other
};

enum class DockWidgetPosition : u8 {
    Left,
    Top,
    Bottom,
    Right
};

enum class SpectrumVisualizerMode : u8 {
    Relative,
    DBFS,
    Equal,
    Waveform
};

enum class Bands : u8 {
    Three = 3,
    Five = 5,
    Ten = 10,
    Eighteen = 18,
    Thirty = 30,
};

ENUM_CONSTANT(Bands, Three, THREE, BANDS);
ENUM_CONSTANT(Bands, Five, FIVE, BANDS);
ENUM_CONSTANT(Bands, Ten, TEN, BANDS);
ENUM_CONSTANT(Bands, Eighteen, EIGHTEEN, BANDS);
ENUM_CONSTANT(Bands, Thirty, THIRTY, BANDS);

enum class SampleSize : u8 {
    U8 = 1,
    S16,
    S24,
    S32
};

enum class AudioChannels : u8 {
    Zero = 0,
    Mono = 1,
    Stereo = 2,
    Quad = 4,
    Surround51 = 6,
    Surround71 = 8,

    // TODO: Support these someday
    Surround102 = 12,
    Surround222 = 24,
};

enum class Associations : u32 {
    None = 0,
    AAC = 1 << 0,
    AC3 = 1 << 1,
    ALAC = 1 << 2,
    FLAC = 1 << 3,
    M4A = 1 << 4,
    MKA = 1 << 5,
    MP3 = 1 << 6,
    OGG = 1 << 7,
    OPUS = 1 << 8,
    WAV = 1 << 9,
    MKV = 1 << 10,
    MOV = 1 << 11,
    MP4 = 1 << 12,
    CUE = 1 << 13,
    M3U = 1 << 14,
    M3U8 = 1 << 15,
    XSPF = 1 << 16,
};
