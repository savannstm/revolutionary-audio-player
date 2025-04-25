#pragma once

#include <cstdint>

enum TrackProperty : std::uint8_t {
    Title,
    Artist,
    Album,
    TrackNumber,
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
    TRACK_PROPERTY_COUNT
};

enum Direction : std::uint8_t {
    Forward,
    BackwardRandom,
    Backward,
    ForwardRandom
};

enum RepeatMode : std::uint8_t {
    Off,
    Track,
    Playlist,
    REPEAT_MODES_COUNT,
};
