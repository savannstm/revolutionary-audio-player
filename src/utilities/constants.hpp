#pragma once

#include "aliases.hpp"
#include "enums.hpp"

#include "frozen/string.h"
#include "frozen/unordered_map.h"

constexpr array<cstr, 9> EXTENSIONS = { ".mp3", ".flac", ".opus",
                                        ".aac", ".wav",  ".ogg",
                                        ".m4a", ".mp4",  ".mkv" };

constexpr u8 SAMPLE_SIZE = sizeof(i16);

constexpr u8 EQ_BANDS_N = 10;
constexpr u8 MAX_VOLUME = 100;

constexpr u8 PROPERTY_COUNT = TrackProperty::Count;

constexpr frozen::unordered_map<frozen::string, TrackProperty, 18>
    TRACK_PROPERTIES = { { "Title", TrackProperty::Title },
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
                         { "File Format", TrackProperty::FileFormat } };