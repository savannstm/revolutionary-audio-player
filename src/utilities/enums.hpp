#pragma once

#include "aliases.hpp"

enum TrackProperty : u8 {
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
    FileFormat,
    Path,
    Cover,
    Count
};

enum Direction : u8 {
    Forward,
    BackwardRandom,
    Backward,
    ForwardRandom
};

enum RepeatMode : u8 {
    Off,
    Track,
    Playlist,
    REPEAT_MODES_N,
};
