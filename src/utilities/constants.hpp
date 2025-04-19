#pragma once

#include "aliases.hpp"
#include "enums.hpp"

constexpr array<cstr, 9> EXTENSIONS = { ".mp3", ".flac", ".opus",
                                        ".aac", ".wav",  ".ogg",
                                        ".m4a", ".mp4",  ".mkv" };

constexpr u8 SAMPLE_SIZE = sizeof(f32);

constexpr u8 PROPERTY_COUNT = TrackProperty::TRACK_PROPERTY_COUNT;

constexpr array<tuple<cstr, TrackProperty>, 19> TRACK_PROPERTIES_MAP = {
    tuple{ "Title", TrackProperty::Title },
    { "Artist", TrackProperty::Artist },
    { "Album", TrackProperty::Album },
    { "Track Number", TrackProperty::TrackNumber },
    { "Album Artist", TrackProperty::AlbumArtist },
    { "Genre", TrackProperty::Genre },
    { "Year", TrackProperty::Year },
    { "Duration", TrackProperty::Duration },
    { "Composer", TrackProperty::Composer },
    { "BPM", TrackProperty::BPM },
    { "Language", TrackProperty::Language },
    { "Disc Number", TrackProperty::DiscNumber },
    { "Comment", TrackProperty::Comment },
    { "Publisher", TrackProperty::Publisher },
    { "Bitrate", TrackProperty::Bitrate },
    { "Sample Rate", TrackProperty::SampleRate },
    { "Channels", TrackProperty::Channels },
    { "File Format", TrackProperty::Format },
    { "Path", TrackProperty::Path },
};

using metadata_array = array<string, PROPERTY_COUNT>;
