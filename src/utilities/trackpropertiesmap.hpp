#include <aliases.hpp>
#include <enums.hpp>

static const array<tuple<cstr, TrackProperty>, TRACK_PROPERTY_COUNT>
    TRACK_PROPERTIES_MAP = {
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