#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"

#include <QSize>

constexpr QLatin1StringView PATHS_FILE_NAME = "/rap-paths.txt"_L1;
constexpr QLatin1StringView PLAYLISTS_FILE_NAME = "/rap-playlists.json"_L1;
constexpr QLatin1StringView SETTINGS_FILE_NAME = "/rap-settings.json"_L1;
constexpr QLatin1StringView PNG_LOGO_PATH = "/rap-logo.png"_L1;
constexpr QLatin1StringView ICO_LOGO_PATH = "/rap-logo.ico"_L1;

// Audio Constants
constexpr i32 INT24_MAX = 8388607;
constexpr i32 INT24_MIN = ~INT24_MAX;
constexpr i32 UINT24_MAX = 16777215;

constexpr u8 F32_SIZE = sizeof(f32);
constexpr u8 I24_SIZE = sizeof(i8) + sizeof(i16);
constexpr u8 I16_SIZE = sizeof(i16);
constexpr f32 I16_DIVISOR = INT16_MAX + 1.0F;
constexpr f32 I24_DIVISOR = INT24_MAX + 1.0F;
constexpr f32 I32_DIVISOR = INT32_MAX + 1.0F;

constexpr u16 FFT_SAMPLE_COUNT = 512;
constexpr u32 FFT_SAMPLE_RATE = 48000;

constexpr u16 MIN_BUFFER_SIZE = 8192;

static_assert(
    MIN_BUFFER_SIZE % 4 == 0,
    "Required to produce 0. Otherwise, samples will be corrupted when playing tracks, since samples may be 16-bit and 32-bit."
);

// Standard EQ band counts

// Frequency sets per band count
constexpr array<f32, THREE_BANDS> THREE_BAND_FREQUENCIES = { 100, 1000, 10000 };

constexpr array<f32, FIVE_BANDS> FIVE_BAND_FREQUENCIES = { 63,
                                                           250,
                                                           1000,
                                                           4000,
                                                           16000 };

constexpr array<f32, TEN_BANDS> TEN_BAND_FREQUENCIES = { 31.5, 63,   125,  250,
                                                         500,  1000, 2000, 4000,
                                                         8000, 16000 };

constexpr array<f32, EIGHTEEN_BANDS> EIGHTEEN_BAND_FREQUENCIES = {
    20,   31.5, 50,   80,   125,  200,  315,   500,   800,
    1250, 2000, 3150, 5000, 6300, 8000, 10000, 12500, 16000
};

constexpr array<f32, THIRTY_BANDS> THIRTY_BAND_FREQUENCIES = {
    25,   31.5, 40,   50,   63,   80,   100,   125,   160,   200,
    250,  315,  400,  500,  630,  800,  1000,  1250,  1600,  2000,
    2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000
};

constexpr auto getFrequenciesForBands(const Bands bands) -> const f32* {
    switch (bands) {
        case Bands::Three:
            return THREE_BAND_FREQUENCIES.data();
        case Bands::Five:
            return FIVE_BAND_FREQUENCIES.data();
        case Bands::Ten:
            return TEN_BAND_FREQUENCIES.data();
        case Bands::Eighteen:
            return EIGHTEEN_BAND_FREQUENCIES.data();
        case Bands::Thirty:
            return THIRTY_BAND_FREQUENCIES.data();
        default:
            return nullptr;
    }
}

// Volume limits
constexpr u8 MAX_VOLUME = 100;
constexpr u8 MIN_VOLUME = 0;

// UI / Layout Constants

// Minimum cover art size
constexpr QSize MINIMUM_COVER_SIZE = QSize(256, 256);

// Search input size constraints
constexpr u8 MINIMUM_MATCHES_NUMBER = 32;

// Playlist / Track Table Config
constexpr static u8 MINIMUM_TRACK_COUNT = 128;
constexpr static u8 MINIMUM_TRACK_TABLE_COLUMN_SECTION_SIZE = 32;

// Supported file extensions

// Music extensions
// Although ALAC uses MPEG containers, let it be here
constexpr QLatin1StringView EXT_AAC = "aac"_L1;
constexpr QLatin1StringView EXT_AC3 = "ac3"_L1;
constexpr QLatin1StringView EXT_ALAC = "alac"_L1;
constexpr QLatin1StringView EXT_FLAC = "flac"_L1;
constexpr QLatin1StringView EXT_M4A = "m4a"_L1;
constexpr QLatin1StringView EXT_MKA = "mka"_L1;
constexpr QLatin1StringView EXT_MP3 = "mp3"_L1;
constexpr QLatin1StringView EXT_OGG = "ogg"_L1;
constexpr QLatin1StringView EXT_OGA = "oga"_L1;
constexpr QLatin1StringView EXT_OGV = "ogv"_L1;
constexpr QLatin1StringView EXT_OGX = "ogx"_L1;
constexpr QLatin1StringView EXT_OPUS = "opus"_L1;
constexpr QLatin1StringView EXT_WAV = "wav"_L1;

constexpr u8 SUPPORTED_AUDIO_EXTENSIONS_COUNT = 13;
constexpr array<QLatin1StringView, SUPPORTED_AUDIO_EXTENSIONS_COUNT>
    SUPPORTED_AUDIO_EXTENSIONS = { EXT_AAC, EXT_AC3, EXT_ALAC, EXT_FLAC,
                                   EXT_M4A, EXT_MKA, EXT_MP3,  EXT_OGG,
                                   EXT_OGA, EXT_OGV, EXT_OGX,  EXT_OPUS,
                                   EXT_WAV };

// Video extensions
constexpr QLatin1StringView EXT_MKV = "mkv"_L1;
constexpr QLatin1StringView EXT_MOV = "mov"_L1;
constexpr QLatin1StringView EXT_MP4 = "mp4"_L1;

constexpr u8 SUPPORTED_VIDEO_EXTENSIONS_COUNT = 3;
constexpr array<QLatin1StringView, SUPPORTED_VIDEO_EXTENSIONS_COUNT>
    SUPPORTED_VIDEO_EXTENSIONS = { EXT_MKV, EXT_MOV, EXT_MP4 };

// Supported playlists
constexpr QLatin1StringView EXT_CUE = "cue"_L1;
constexpr QLatin1StringView EXT_M3U = "m3u"_L1;
constexpr QLatin1StringView EXT_M3U8 = "m3u8"_L1;
constexpr QLatin1StringView EXT_XSPF = "xspf"_L1;

constexpr u8 SUPPORTED_PLAYLIST_EXTENSIONS_COUNT = 4;
constexpr array<QLatin1StringView, SUPPORTED_PLAYLIST_EXTENSIONS_COUNT>
    SUPPORTED_PLAYLIST_EXTENSIONS = { EXT_CUE, EXT_M3U, EXT_M3U8, EXT_XSPF };

// All supported playable file extensions
constexpr u8 SUPPORTED_EXTENSIONS_COUNT = SUPPORTED_AUDIO_EXTENSIONS_COUNT +
                                          SUPPORTED_VIDEO_EXTENSIONS_COUNT +
                                          SUPPORTED_PLAYLIST_EXTENSIONS_COUNT;
constexpr array<QLatin1StringView, SUPPORTED_EXTENSIONS_COUNT>
    SUPPORTED_PLAYABLE_EXTENSIONS = {
        // Audio
        EXT_AAC,
        EXT_AC3,
        EXT_ALAC,
        EXT_FLAC,
        EXT_M4A,
        EXT_MKA,
        EXT_MP3,
        EXT_OGG,
        EXT_OGA,
        EXT_OGV,
        EXT_OGX,
        EXT_OPUS,
        EXT_WAV,

        // Video
        EXT_MKV,
        EXT_MOV,
        EXT_MP4,

        // Playlist
        EXT_CUE,
        EXT_M3U,
        EXT_M3U8,
        EXT_XSPF
    };

// Image extensions
constexpr array<QLatin1StringView, 6> SUPPORTED_IMAGE_EXTENSIONS = {
    "bmp"_L1, "jpeg"_L1, "jpg"_L1, "png"_L1, "tiff"_L1, "webp"_L1
};

// Time and data unit conversions
constexpr u16 SECOND_MS = 1000;
constexpr u16 KB_BYTES = 1000;
constexpr u8 MINUTE_SECONDS = 60;
constexpr u16 HOUR_SECONDS = 3600;
constexpr u8 START_DRAG_DISTANCE = 40;

constexpr u16 STATUSBAR_MSG_TIMEOUT = 2000;

struct TrackPropertyInfo {
    const array<const cstr, 4> ffmpegTags;
    const cstr xspfTag;
    const cstr label;
    const QLatin1StringView tag;
};

constexpr array<TrackPropertyInfo, TRACK_PROPERTY_COUNT>
    TRACK_PROPERTIES_INFOS = { {
        { .ffmpegTags = { "album" },
          .xspfTag = "album",
          .label = QT_TR_NOOP("Album"),
          .tag = "album"_L1 },
        { .ffmpegTags = { "album_artist", "albumartist" },
          .xspfTag = "albumArtist",
          .label = QT_TR_NOOP("Album Artist"),
          .tag = "albumartist"_L1 },
        { .ffmpegTags = { "arranger" },
          .label = QT_TR_NOOP("Arranger"),
          .tag = "arranger"_L1 },
        { .ffmpegTags = { "artist" },
          .xspfTag = "creator",
          .label = QT_TR_NOOP("Artist"),
          .tag = "artist"_L1 },
        { .xspfTag = "format",
          .label = QT_TR_NOOP("Format"),
          .tag = "format"_L1 },
        { .xspfTag = "bitrate",
          .label = QT_TR_NOOP("Bitrate"),
          .tag = "bitrate"_L1 },
        { .ffmpegTags = { "bpm", "tbpm", "ibpm" },
          .xspfTag = "bpm",
          .label = QT_TR_NOOP("BPM"),
          .tag = "bpm"_L1 },
        { .ffmpegTags = { "catalognumber", "catalog_number" },
          .label = QT_TR_NOOP("Catalog Number"),
          .tag = "catalog"_L1 },
        { .xspfTag = "channels",
          .label = QT_TR_NOOP("Channels"),
          .tag = "channels"_L1 },
        { .ffmpegTags = { "comment", "description" },
          .label = QT_TR_NOOP("Comment"),
          .tag = "comment"_L1 },
        { .ffmpegTags = { "compilation" },
          .label = QT_TR_NOOP("Compilation"),
          .tag = "compilation"_L1 },
        { .ffmpegTags = { "composer" },
          .xspfTag = "composer",
          .label = QT_TR_NOOP("Composer"),
          .tag = "composer"_L1 },
        { .ffmpegTags = { "conductor" },
          .label = QT_TR_NOOP("Conductor"),
          .tag = "conductor"_L1 },
        { .ffmpegTags = { "cover", "covr" },
          .label = QT_TR_NOOP("Cover Art"),
          .tag = "cover"_L1 },
        { .ffmpegTags = { "disc", "discnumber" },
          .xspfTag = "disc",
          .label = QT_TR_NOOP("Disc Number"),
          .tag = "disc"_L1 },
        { .xspfTag = "duration",
          .label = QT_TR_NOOP("Duration"),
          .tag = "duration"_L1 },
        { .ffmpegTags = { "feat", "featuring" },
          .label = QT_TR_NOOP("Featuring Artists"),
          .tag = "featuring"_L1 },
        { .ffmpegTags = { "genre" },
          .xspfTag = "genre",
          .label = QT_TR_NOOP("Genre"),
          .tag = "genre"_L1 },
        { .ffmpegTags = { "grouping" },
          .label = QT_TR_NOOP("Grouping"),
          .tag = "grouping"_L1 },
        { .ffmpegTags = { "isrc" },
          .label = QT_TR_NOOP("ISRC"),
          .tag = "isrc"_L1 },
        { .ffmpegTags = { "label" },
          .label = QT_TR_NOOP("Label"),
          .tag = "label"_L1 },
        { .ffmpegTags = { "language" },
          .xspfTag = "language",
          .label = QT_TR_NOOP("Language"),
          .tag = "language"_L1 },
        { .ffmpegTags = { "lastplayed" },
          .label = QT_TR_NOOP("Last Played"),
          .tag = "lastplayed"_L1 },
        { .ffmpegTags = { "lyricist", "lyrics_by", "author" },
          .label = QT_TR_NOOP("Lyricist"),
          .tag = "lyricist"_L1 },
        { .ffmpegTags = { "lyrics", "unsyncedlyrics" },
          .label = QT_TR_NOOP("Lyrics"),
          .tag = "lyrics"_L1 },
        { .label = QT_TR_NOOP("Order"), .tag = "order"_L1 },
        { .ffmpegTags = { "originalartist" },
          .label = QT_TR_NOOP("Original Artist"),
          .tag = "originalartist"_L1 },
        { .ffmpegTags = { "originaldate", "original_release_date" },
          .label = QT_TR_NOOP("Original Release Date"),
          .tag = "originaldate"_L1 },
        { .xspfTag = "location",
          .label = QT_TR_NOOP("Path"),
          .tag = "path"_L1 },
        { .ffmpegTags = { "playcount" },
          .label = QT_TR_NOOP("Play Count"),
          .tag = "playcount"_L1 },
        { .ffmpegTags = { "producer" },
          .label = QT_TR_NOOP("Producer"),
          .tag = "producer"_L1 },
        { .ffmpegTags = { "publisher" },
          .xspfTag = "publisher",
          .label = QT_TR_NOOP("Publisher"),
          .tag = "publisher"_L1 },
        { .ffmpegTags = { "rating" },
          .label = QT_TR_NOOP("Rating"),
          .tag = "rating"_L1 },
        { .ffmpegTags = { "release_date", "releasedate" },
          .label = QT_TR_NOOP("Release Date"),
          .tag = "releasedate"_L1 },
        { .ffmpegTags = { "remixer" },
          .label = QT_TR_NOOP("Remixer"),
          .tag = "remixer"_L1 },
        { .label = QT_TR_NOOP("Sample Format"), .tag = "sampleformat"_L1 },
        { .xspfTag = "samplerate",
          .label = QT_TR_NOOP("Sample Rate"),
          .tag = "samplerate"_L1 },
        { .ffmpegTags = { "subtitle" },
          .label = QT_TR_NOOP("Subtitle"),
          .tag = "subtitle"_L1 },
        { .ffmpegTags = { "title" },
          .xspfTag = "title",
          .label = QT_TR_NOOP("Title"),
          .tag = "title"_L1 },
        { .ffmpegTags = { "totaldiscs", "disctotal" },
          .label = QT_TR_NOOP("Total Discs"),
          .tag = "totaldiscs"_L1 },
        { .ffmpegTags = { "totaltracks", "tracktotal" },
          .label = QT_TR_NOOP("Total Tracks"),
          .tag = "totaltracks"_L1 },
        { .ffmpegTags = { "track", "tracknumber" },
          .xspfTag = "trackNum",
          .label = QT_TR_NOOP("Track Number"),
          .tag = "track"_L1 },
        { .ffmpegTags = { "date", "year" },
          .xspfTag = "year",
          .label = QT_TR_NOOP("Year"),
          .tag = "year"_L1 },
    } };

static_assert(
    TRACK_PROPERTIES_INFOS.size() == TRACK_PROPERTY_COUNT,
    "Track properties info does not contains some entries from TrackProperty enum."
);

struct ColumnSettings {
    u8 index = 0;
    bool hidden = true;
};

using ColumnSettingsArray = array<ColumnSettings, TRACK_PROPERTY_COUNT>;

consteval auto defaultColumnSettings() -> ColumnSettingsArray {
    ColumnSettingsArray arr;
    u8 index = 0;

    arr[u8(TrackProperty::Title)].hidden = false;
    arr[u8(TrackProperty::Title)].index = 0;

    arr[u8(TrackProperty::Artist)].hidden = false;
    arr[u8(TrackProperty::Artist)].index = 1;

    arr[u8(TrackProperty::Duration)].hidden = false;
    arr[u8(TrackProperty::Duration)].index = 2;

    for (const auto prop : TRACK_PROPERTIES) {
        if (prop != TrackProperty::Title && prop != TrackProperty::Artist &&
            prop != TrackProperty::Duration) {
            arr[u8(prop)].index = index++;
        }
    }

    return arr;
}

constexpr ColumnSettingsArray DEFAULT_COLUMN_SETTINGS = defaultColumnSettings();
