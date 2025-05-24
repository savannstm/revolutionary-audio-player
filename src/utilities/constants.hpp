#pragma once

#include "aliases.hpp"
#include "enums.hpp"

#include <libavutil/samplefmt.h>

#include <QSize>

// Audio Constants
constexpr u16 MIN_BUFFER_SIZE = 4096;
constexpr u16 MIN_BUFFER_SIZE_I24 = 4095;

constexpr u8 F32_SAMPLE_SIZE = sizeof(f32);
constexpr AVSampleFormat F32_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLT;
constexpr cstr F32_SAMPLE_FORMAT_NAME = "flt";

// Equalizer gain range (in dB)
constexpr i8 MAX_GAIN = 20;
constexpr i8 MIN_GAIN = -20;

// Frequency limits (in Hz)
constexpr f32 MAX_FREQUENCY = 20000;
constexpr f32 MIN_FREQUENCY = 0;

// Standard EQ band counts
constexpr u8 MAX_BANDS_COUNT = 30;
constexpr u8 THIRTY_BANDS = 30;
constexpr u8 EIGHTEEN_BANDS = 18;
constexpr u8 TEN_BANDS = 10;
constexpr u8 FIVE_BANDS = 5;
constexpr u8 THREE_BANDS = 3;

// Bandwidths for band counts
constexpr u16 THIRTY_BANDS_WIDTH = 125;
constexpr u16 EIGHTEEN_BANDS_WIDTH = 250;
constexpr u16 TEN_BANDS_WIDTH = 500;
constexpr u16 FIVE_BANDS_WIDTH = 1000;
constexpr u16 THREE_BANDS_WIDTH = 2000;

// Frequency sets per band count
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

// Standard bitrates (in kbps)
constexpr array<u16, 17> STANDARD_BITRATES = { 32,  40,  48,  56,  64,  80,
                                               96,  112, 128, 160, 192, 224,
                                               256, 320, 350, 450, 510 };
constexpr u16 MAX_LOSSY_BITRATE =
    STANDARD_BITRATES[STANDARD_BITRATES.size() - 1];
constexpr u16 MIN_LOSSY_BITRATE = STANDARD_BITRATES[0];

// Volume limits
constexpr u8 MAX_VOLUME = 100;
constexpr u8 MIN_VOLUME = 0;

// UI / Layout Constants

// Transparency and blur settings
constexpr f32 HALF_TRANSPARENT = 0.5;
constexpr f32 BLUR_SIGMA = 10;

// Minimum cover art size
constexpr QSize MINIMUM_COVER_SIZE = QSize(256, 256);

// Playlist tab layout
constexpr QMargins TAB_MARGINS = { 8, 4, 8, 4 };
constexpr u8 TAB_LABEL_RIGHT_MARGIN = 8;
constexpr QSize PLAYLIST_TAB_CLOSE_BUTTON_SIZE = QSize(16, 16);

// Header handle width (used for resizing column headers)
constexpr u8 HEADER_HANDLE_WIDTH = 4;

// Search input size constraints
constexpr u8 SEARCH_INPUT_MIN_WIDTH = 64;
constexpr u8 SEARCH_INPUT_HEIGHT = 24;

// Equalizer menu input widths

constexpr u8 GAIN_INPUT_FIXED_WIDTH = 32;
constexpr u8 FREQUENCY_INPUT_FIXED_WIDTH = 48;

// Playlist / Track Tree Config

// Minimum number of tracks to reserve for collections that hold tracks
constexpr u8 MINIMUM_TRACK_COUNT = 128;

// Minimum column section size in track tree (px)
constexpr u8 MINIMUM_TRACK_TREE_COLUMN_SECTION_SIZE = 26;

// Row height in track tree (px)
constexpr u8 TRACK_TREE_ROW_HEIGHT = 18;

// Role for custom properties in Qt models
constexpr u16 PROPERTY_ROLE = Qt::UserRole + 1;

// Default visible columns in track tree (true = visible)
constexpr array<bool, TRACK_PROPERTY_COUNT> DEFAULT_COLUMN_STATES = {
    false, false, false, false, true, true, true, true, true, true,
    true,  true,  true,  true,  true, true, true, true, true, true
};

// Prevent mismatch fuckup
consteval auto constructProperties()
    -> array<TrackProperty, TRACK_PROPERTY_COUNT> {
    array<TrackProperty, TRACK_PROPERTY_COUNT> properties;

    for (const u8 property : range(0, TRACK_PROPERTY_COUNT)) {
        properties[property] = as<TrackProperty>(property);
    }

    return properties;
}

// Default property ordering for track tree columns
constexpr array<TrackProperty, TRACK_PROPERTY_COUNT> DEFAULT_COLUMN_PROPERTIES =
    constructProperties();

// Metadata / Search Config

// Allowed file extensions for tracks
// Although ALAC uses MPEG containers, let it be here
constexpr QStringView EXT_MP3 = u"mp3";
constexpr QStringView EXT_FLAC = u"flac";
constexpr QStringView EXT_OPUS = u"opus";
constexpr QStringView EXT_AAC = u"aac";
constexpr QStringView EXT_WAV = u"wav";
constexpr QStringView EXT_OGG = u"ogg";
constexpr QStringView EXT_M4A = u"m4a";
constexpr QStringView EXT_MP4 = u"mp4";
constexpr QStringView EXT_MKV = u"mkv";
constexpr QStringView EXT_MKA = u"mka";
constexpr QStringView EXT_ALAC = u"alac";
constexpr QStringView EXT_MOV = u"mov";

constexpr u8 ALLOWED_FILE_EXTENSIONS_COUNT = 12;
constexpr array<QStringView, ALLOWED_FILE_EXTENSIONS_COUNT>
    ALLOWED_FILE_EXTENSIONS = {
        EXT_MP3, EXT_FLAC, EXT_OPUS, EXT_AAC, EXT_WAV,  EXT_OGG,
        EXT_M4A, EXT_MP4,  EXT_MKV,  EXT_MKA, EXT_ALAC, EXT_MOV,
    };

constexpr u8 ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT = 9;
constexpr array<QStringView, ALLOWED_MUSIC_FILE_EXTENSIONS_COUNT>
    ALLOWED_MUSIC_FILE_EXTENSIONS = {
        EXT_MP3, EXT_FLAC, EXT_OPUS, EXT_AAC,  EXT_WAV,
        EXT_OGG, EXT_M4A,  EXT_MKA,  EXT_ALAC,
    };

constexpr u8 ALLOWED_VIDEO_FILE_EXTENSIONS_COUNT = 3;
constexpr array<QStringView, ALLOWED_VIDEO_FILE_EXTENSIONS_COUNT>
    ALLOWED_VIDEO_FILE_EXTENSIONS = { EXT_MP4, EXT_MKV, EXT_MOV };

// Searchable track property names
constexpr array<QStringView, TRACK_PROPERTY_COUNT> SEARCH_PROPERTIES = {
    u"",           u"title",       u"artist",    u"no",
    u"album",      u"albumartist", u"genre",     u"year",
    u"duration",   u"composer",    u"bpm",       u"language",
    u"discnumber", u"comment",     u"publisher", u"bitrate",
    u"samplerate", u"channels",    u"format",    u"path"
};

// Minimum number of search result matches
constexpr u8 MINIMUM_MATCHES_NUMBER = 32;

// Application Config

// Available application style names
constexpr array<QStringView, 8> APPLICATION_STYLES = {
    u"windows11", u"windowsvista", u"Windows", u"Fusion",
    u"macintosh", u"breeze",       u"gtk",     u"adwaita"
};

// Time and data unit conversions
constexpr u16 SECOND_MS = 1000;
constexpr u16 KB_BYTES = 1000;
constexpr u8 MINUTE_SECONDS = 60;

// Type Aliases / Structs

constexpr i32 INT24_SIZE = 3;
constexpr i32 INT24_MAX = 8388607;
constexpr i32 INT24_MIN = ~INT24_MAX;
constexpr i32 UINT24_MAX = 16777215;

using MetadataArray = array<QString, TRACK_PROPERTY_COUNT>;
using GainArray = array<i8, MAX_BANDS_COUNT>;
using FrequencyArray = array<f32, MAX_BANDS_COUNT>;

// Equalizer settings info struct
struct EqualizerInfo {
    const bool enabled;
    const u8 bandIndex;
    const GainArray gains;
    const FrequencyArray frequencies;
};
