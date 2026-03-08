#pragma once

#include "Aliases.hpp"

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

#define ENUM_CONSTANT(EnumType, Member, NAME, SUFFIX) \
    constexpr auto NAME##_##SUFFIX = \
        static_cast<std::underlying_type_t<EnumType>>(EnumType::Member)

enum class TrackProperty : u8 {
    Album,
    AlbumArtist,
    Arranger,
    Artist,
    AudioFormat,
    Bitrate,
    BPM,
    CatalogNumber,
    Channels,
    Comment,
    Compilation,
    Composer,
    Conductor,
    CoverArt,
    DiscNumber,
    Duration,
    FeaturingArtists,
    Genre,
    Grouping,
    ISRC,
    Label,
    Language,
    LastPlayed,
    Lyricist,
    Lyrics,
    Order,
    OriginalArtist,
    OriginalReleaseDate,
    Path,
    PlayCount,
    Producer,
    Publisher,
    Rating,
    ReleaseDate,
    Remixer,
    SampleFormat,
    SampleRate,
    Subtitle,
    Title,
    TotalDiscs,
    TotalTracks,
    TrackNumber,
    Year
};

constexpr u8 TRACK_PROPERTY_COUNT = magic_enum::enum_count<TrackProperty>();
constexpr auto TRACK_PROPERTIES = magic_enum::enum_values<TrackProperty>();

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

enum Bands : u8 {
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

constexpr Bands MAX_BANDS = *(magic_enum::enum_values<Bands>().end() - 1);

enum SampleSize : u8 {
    S16 = 2,
    S24,
    // Alias for f32
    S32
};

//! OGM is not an official standard by Xiph.org, so we'll never support it
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
    OGA = 1 << 8,
    OGV = 1 << 9,
    OGX = 1 << 10,
    OPUS = 1 << 11,
    WAV = 1 << 12,

    MKV = 1 << 13,
    MOV = 1 << 14,
    MP4 = 1 << 15,

    CUE = 1 << 16,
    M3U = 1 << 17,
    M3U8 = 1 << 18,
    XSPF = 1 << 19,
};

enum class ProgressDisplayMode : u8 {
    Elapsed,
    Remaining,
};

enum class TreeStatus : u8 {
    Idle,
    Suspended,
    Playing
};

enum class AdvanceStatus : u8 {
    PlaylistRepeated,
    TrackRepeated,
    TrackFinished,
    PlaylistFinished,
    PlaybackFinished,
};