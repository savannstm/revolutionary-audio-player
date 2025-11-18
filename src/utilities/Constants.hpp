#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"

#include <libavutil/samplefmt.h>

#include <QSize>

constexpr QStringView PATHS_FILE_NAME = u"rap-paths.txt"_qsv;
constexpr QStringView PLAYLISTS_FILE_NAME = u"rap-playlists.json"_qsv;
constexpr QStringView SETTINGS_FILE_NAME = u"rap-settings.json"_qsv;
constexpr QStringView PNG_LOGO_PATH = u"icons/rap-logo.png"_qsv;
constexpr QStringView PNG_LOGO32_PATH = u"icons/rap-logo-32px.png"_qsv;
constexpr QStringView ICO_LOGO_PATH = u"icons/rap-logo.ico"_qsv;

// Audio Constants
enum SampleSize : u8 {
    U8 = 1,
    S16,
    S24,
    S32
};

enum AudioChannels : u8 {
    Zero = 0,
    Mono = 1,
    Stereo = 2,
    Quad = 4,
    Surround51 = 6,
    Surround71 = 8,
};

constexpr u8 F32_SAMPLE_SIZE = sizeof(f32);

constexpr u16 DESIRED_SAMPLE_COUNT = 2048;
constexpr u16 MIN_BUFFER_SIZE = DESIRED_SAMPLE_COUNT * F32_SAMPLE_SIZE;

static_assert(
    MIN_BUFFER_SIZE % 4 == 0,
    "Required to produce 0. Otherwise, samples will be corrupted when playing tracks, since samples may be 16-bit and 32-bit."
);

constexpr u16 MIN_BUFFER_SIZE_3BYTES = MIN_BUFFER_SIZE - (MIN_BUFFER_SIZE / 4);

constexpr AVSampleFormat F32_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLT;
constexpr AVSampleFormat F32P_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLTP;

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

constexpr auto getFrequenciesForBands(const u8 bands) -> const f32* {
    switch (bands) {
        case THREE_BANDS:
            return THREE_BAND_FREQUENCIES.data();
        case FIVE_BANDS:
            return FIVE_BAND_FREQUENCIES.data();
        case TEN_BANDS:
            return TEN_BAND_FREQUENCIES.data();
        case EIGHTEEN_BANDS:
            return EIGHTEEN_BAND_FREQUENCIES.data();
        case THIRTY_BANDS:
            return THIRTY_BAND_FREQUENCIES.data();
        default:
            return nullptr;
    }
}

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

// Roles for CUE tracks
constexpr u16 CUE_OFFSET_ROLE = Qt::UserRole + 2;
constexpr u16 CUE_FILE_PATH_ROLE = Qt::UserRole + 3;

consteval auto constructStates() -> array<bool, TRACK_PROPERTY_COUNT> {
    array<bool, TRACK_PROPERTY_COUNT> properties;

    for (const u8 property : range(0, TRACK_PROPERTY_COUNT)) {
        properties[property] = property > 3;
    }

    return properties;
}

consteval auto constructProperties()
    -> array<TrackProperty, TRACK_PROPERTY_COUNT> {
    array<TrackProperty, TRACK_PROPERTY_COUNT> properties;

    for (const u8 property : range(0, TRACK_PROPERTY_COUNT)) {
        properties[property] = TrackProperty(property);
    }

    return properties;
}

// Default visible columns in track tree (true = visible)
constexpr array<bool, TRACK_PROPERTY_COUNT> DEFAULT_COLUMN_STATES =
    constructStates();

// Default property ordering for track tree columns
constexpr array<TrackProperty, TRACK_PROPERTY_COUNT> DEFAULT_COLUMN_PROPERTIES =
    constructProperties();

// Allowed file extensions for tracks

// Music extensions
// Although ALAC uses MPEG containers, let it be here
constexpr QStringView EXT_AAC = u"aac";
constexpr QStringView EXT_AC3 = u"ac3";
constexpr QStringView EXT_ALAC = u"alac";
constexpr QStringView EXT_FLAC = u"flac";
constexpr QStringView EXT_M4A = u"m4a";
constexpr QStringView EXT_MKA = u"mka";
constexpr QStringView EXT_MP3 = u"mp3";
constexpr QStringView EXT_OGG = u"ogg";
constexpr QStringView EXT_OPUS = u"opus";
constexpr QStringView EXT_WAV = u"wav";

constexpr u8 ALLOWED_AUDIO_EXTENSIONS_COUNT = 10;
constexpr array<QStringView, ALLOWED_AUDIO_EXTENSIONS_COUNT>
    ALLOWED_AUDIO_EXTENSIONS = {
        EXT_AAC, EXT_AC3, EXT_ALAC, EXT_FLAC, EXT_M4A,
        EXT_MKA, EXT_MP3, EXT_OGG,  EXT_OPUS, EXT_WAV
    };

// Video extensions
constexpr QStringView EXT_MKV = u"mkv";
constexpr QStringView EXT_MOV = u"mov";
constexpr QStringView EXT_MP4 = u"mp4";

constexpr u8 ALLOWED_VIDEO_EXTENSIONS_COUNT = 3;
constexpr array<QStringView, ALLOWED_VIDEO_EXTENSIONS_COUNT>
    ALLOWED_VIDEO_EXTENSIONS = { EXT_MKV, EXT_MOV, EXT_MP4 };

// Allowed playlists
constexpr QStringView EXT_CUE = u"cue";
constexpr QStringView EXT_M3U = u"m3u";
constexpr QStringView EXT_M3U8 = u"m3u8";
constexpr QStringView EXT_XSPF = u"xspf";

constexpr u8 ALLOWED_PLAYLIST_EXTENSIONS_COUNT = 4;
constexpr array<QStringView, ALLOWED_PLAYLIST_EXTENSIONS_COUNT>
    ALLOWED_PLAYLIST_EXTENSIONS = { EXT_CUE, EXT_M3U, EXT_M3U8, EXT_XSPF };

// All allowed playable file extensions
constexpr u8 ALLOWED_EXTENSIONS_COUNT = ALLOWED_AUDIO_EXTENSIONS_COUNT +
                                        ALLOWED_VIDEO_EXTENSIONS_COUNT +
                                        ALLOWED_PLAYLIST_EXTENSIONS_COUNT;
constexpr array<QStringView, ALLOWED_EXTENSIONS_COUNT>
    ALLOWED_PLAYABLE_EXTENSIONS = {
        // Audio
        EXT_AAC,
        EXT_AC3,
        EXT_ALAC,
        EXT_FLAC,
        EXT_M4A,
        EXT_MKA,
        EXT_MP3,
        EXT_OGG,
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
constexpr array<QStringView, 6> ALLOWED_IMAGE_EXTENSIONS = { u"bmp",  u"jpeg",
                                                             u"jpg",  u"png",
                                                             u"tiff", u"webp" };

// Searchable track property names
constexpr array<QStringView, TRACK_PROPERTY_COUNT> SEARCH_PROPERTIES = {
    u"",           u"title",       u"artist",    u"no",
    u"album",      u"albumartist", u"genre",     u"year",
    u"duration",   u"composer",    u"bpm",       u"language",
    u"discnumber", u"comment",     u"publisher", u"bitrate",
    u"samplerate", u"channels",    u"format",    u"path",
    u"order"
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
constexpr u16 HOUR_SECONDS = 3600;
constexpr u8 START_DRAG_DISTANCE = 40;

constexpr i32 INT24_MAX = 8388607;
constexpr i32 INT24_MIN = ~INT24_MAX;
constexpr i32 UINT24_MAX = 16777215;

constexpr array<cstr, 3> BPM_TAGS = { "BPM", "TBPM", "IBPM" };

// Visualizer
constexpr cstr VISUALIZER_SHARED_MEMORY_LABEL = "rap-visualizer";

constexpr u16 MIN_MESH_SIZE = 10;
constexpr u16 MAX_MESH_SIZE = 200;

constexpr u16 DEFAULT_MESH_WIDTH = 80;
constexpr u16 DEFAULT_MESH_HEIGHT = 40;

constexpr u16 MIN_FPS = 24;
constexpr u16 MAX_FPS = 360;

constexpr u16 PRESET_PATH_LIMIT = 512;

struct VisualizerSharedData {
    array<f32, DESIRED_SAMPLE_COUNT> audioBuffer;
    array<char, PRESET_PATH_LIMIT> presetPath;

    atomic<u16> bufferSize = MIN_BUFFER_SIZE;
    atomic<u16> fps = MIN_FPS;
    atomic<u16> meshWidth = DEFAULT_MESH_WIDTH;
    atomic<u16> meshHeight = DEFAULT_MESH_HEIGHT;

    atomic<bool> loadPreset = false;
    atomic<bool> presetRequested = false;

    atomic<bool> running = true;
    atomic<bool> hasNewData = false;

    atomic<AudioChannels> channels = AudioChannels::Zero;
};