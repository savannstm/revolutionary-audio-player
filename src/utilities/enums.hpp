#pragma once

#include "aliases.hpp"

enum TrackProperty : u8 {
    Play,
    Title,
    Artist,
    TrackNumber,
    Album,
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
    Order,
    TRACK_PROPERTY_COUNT
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
    REPEAT_MODES_COUNT,
};

enum DragDropMode : u8 {
    CreateNewPlaylist,
    AddToCurrentPlaylist
};

enum PlaylistNaming : u8 {
    DirectoryName,
    TrackMetadata,
    Numbered
};

enum Style : u8 {
    Windows11,
    WindowsVista,
    Windows,
    Fusion,
    Macintosh,
    Breeze,
    GTK,
    Adwaita
};

enum PlaylistFileType : u8 {
    XSPF,
    M3U,
    CUE
};

enum TabRemoveMode : u8 {
    Single,
    ToLeft,
    ToRight,
    Other
};

enum DockWidgetPosition : u8 {
    Left,
    Top,
    Bottom,
    Right
};

enum PeakVisualizerMode : u8 {
    Relative,
    DBFS
};