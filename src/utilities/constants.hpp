#pragma once

#include "aliases.hpp"
#include "enums.hpp"

#include <QSize>

constexpr u8 CHANNEL_COUNT = 2;
constexpr f32 Q_FACTOR = 1;

constexpr usize SAMPLE_SIZE = sizeof(f32);

constexpr i8 MAX_DB = 20;
constexpr i8 MIN_DB = -20;
constexpr u8 GAIN_INPUT_FIXED_WIDTH = 32;

constexpr u8 MINIMUM_MUSIC_MODEL_TRACK_COUNT = 128;

constexpr f32 HALF_TRANSPARENT = 0.5;
constexpr f32 BLUR_SIGMA = 10;
constexpr u8 MINIMUM_TRACK_TREE_COLUMN_SECTION_SIZE = 26;

constexpr QSize MINIMUM_COVER_SIZE = QSize(256, 256);

constexpr QMargins TAB_MARGINS = { 8, 4, 8, 4 };
constexpr u8 TAB_LABEL_RIGHT_MARGIN = 8;

constexpr u8 HEADER_HANDLE_WIDTH = 4;
constexpr QSize PLAYLIST_TAB_CLOSE_BUTTON_SIZE = QSize(16, 16);

constexpr u8 MINIMUM_MATCHES_NUMBER = 32;
constexpr u8 SEARCH_INPUT_MIN_WIDTH = 64;
constexpr u8 SEARCH_INPUT_HEIGHT = 24;

constexpr u8 THIRTY_BANDS = 30;
constexpr u8 EIGHTEEN_BANDS = 18;
constexpr u8 TEN_BANDS = 10;
constexpr u8 FIVE_BANDS = 5;
constexpr u8 THREE_BANDS = 3;

constexpr array<f32, THIRTY_BANDS> THIRTY_BAND_FREQUENCIES = {
    25,   31.5, 40,   50,   63,   80,   100,   125,   160,   200,
    250,  315,  400,  500,  630,  800,  1000,  1250,  1600,  2000,
    2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000
};

constexpr array<f32, EIGHTEEN_BANDS> EIGHTEEN_BAND_FREQUENCIES = {
    20,   31.5, 50,   80,   125,  200,  315,   500,   800,
    1250, 2000, 3150, 5000, 6300, 8000, 10000, 12500, 16000
};

constexpr array<f32, TEN_BANDS> TEN_BAND_FREQUENCIES = { 31.5, 63,   125,  250,
                                                         500,  1000, 2000, 4000,
                                                         8000, 16000 };

constexpr array<f32, FIVE_BANDS> FIVE_BAND_FREQUENCIES = { 63,
                                                           250,
                                                           1000,
                                                           4000,
                                                           16000 };

constexpr array<f32, THREE_BANDS> THREE_BAND_FREQUENCIES = { 100, 1000, 10000 };

constexpr u16 PROPERTY_ROLE = Qt::UserRole + 1;

constexpr array<QStringView, 10> ALLOWED_FILE_EXTENSIONS = {
    u"mp3", u"flac", u"opus", u"aac", u"wav",
    u"ogg", u"m4a",  u"mp4",  u"mkv", u"mka"
};

constexpr array<QStringView, TRACK_PROPERTY_COUNT> SEARCH_PROPERTIES = {
    u"",           u"title",       u"artist",    u"no",
    u"album",      u"albumartist", u"genre",     u"year",
    u"duration",   u"composer",    u"bpm",       u"language",
    u"discnumber", u"comment",     u"publisher", u"bitrate",
    u"samplerate", u"channels",    u"format",    u"path"
};

constexpr array<u16, 18> STANDARD_BITRATES = { 32,  40,  48,  56,  64,  80,
                                               96,  112, 128, 160, 192, 224,
                                               256, 320, 350, 384, 448, 510 };

constexpr array<TrackProperty, TRACK_PROPERTY_COUNT>
    TRACK_TREE_DEFAULT_COLUMN_PROPERTIES = {
        Play,        Title,      Artist,     TrackNumber, Album,
        AlbumArtist, Genre,      Year,       Duration,    Composer,
        BPM,         Language,   DiscNumber, Comment,     Publisher,
        Bitrate,     SampleRate, Channels,   Format,      Path,
    };

constexpr array<bool, TRACK_PROPERTY_COUNT> TRACK_TREE_DEFAULT_COLUMN_STATES = {
    false, false, false, false, true, true, true, true, true, true,
    true,  true,  true,  true,  true, true, true, true, true, true
};

constexpr u8 MAX_VOLUME = 100;
constexpr u8 MIN_VOLUME = 0;

constexpr u8 TRACK_TREE_ROW_HEIGHT = 18;

constexpr u16 SECOND_MS = 1000;
constexpr u16 KB_BYTES = 1000;

constexpr u8 MINUTE_SECONDS = 60;

constexpr array<QStringView, 8> APPLICATION_STYLES = {
    u"windows11", u"windowsvista", u"Windows", u"Fusion",
    u"macintosh", u"breeze",       u"gtk",     u"adwaita"
};

using metadata_array = array<QString, TRACK_PROPERTY_COUNT>;

struct EqualizerInfo {
    const bool enabled;
    const u8 bandIndex;
    const array<i8, THIRTY_BANDS> gains;
    const array<f32, THIRTY_BANDS> frequencies;
};