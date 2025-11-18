#pragma once

#include "Aliases.hpp"

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
    Order,
    COUNT
};

constexpr u8 TRACK_PROPERTY_COUNT = u8(TrackProperty::COUNT);

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

enum class Style : u8 {
    Windows11,
    WindowsVista,
    Windows,
    Fusion,
    Macintosh,
    Breeze,
    GTK,
    Adwaita
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

enum class PeakVisualizerMode : u8 {
    Relative,
    DBFS
};

enum class Bands : u8 {
    Three = 3,
    Five = 5,
    Ten = 10,
    Eighteen = 18,
    Thirty = 30,
};

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

constexpr auto operator|(Associations lhs, Associations rhs) -> Associations {
    return Associations(u32(lhs) | u32(rhs));
}

constexpr auto operator&(Associations lhs, Associations rhs) -> bool {
    return bool(u32(lhs) & u32(rhs));
}

constexpr auto operator|=(Associations& lhs, Associations rhs)
    -> Associations& {
    lhs = lhs | rhs;
    return lhs;
}

constexpr auto operator&=(Associations& lhs, Associations rhs)
    -> Associations& {
    lhs = Associations(u32(lhs) & u32(rhs));
    return lhs;
}
