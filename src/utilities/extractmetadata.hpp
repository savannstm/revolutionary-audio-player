#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

#include "aliases.hpp"
#include "constants.hpp"
#include "tominutes.hpp"

#include <QDebug>
#include <algorithm>
#include <fstream>

constexpr array<u16, 18> STANDARD_BITRATES = { 32,  40,  48,  56,  64,  80,
                                               96,  112, 128, 160, 192, 224,
                                               256, 320, 350, 384, 448, 510 };

auto roundBitrate(const u32 bitrate) -> string {
    const u32 kbps = bitrate / 1000;

    u16 closest = STANDARD_BITRATES[0];
    u32 minDiff = abs(static_cast<i32>(kbps - closest));

    for (const u16 bitrate : STANDARD_BITRATES) {
        const u32 diff = abs(static_cast<i32>(kbps - bitrate));

        if (diff < minDiff) {
            minDiff = diff;
            closest = bitrate;
        }
    }

    return format("{}k", closest);
}

auto extractMetadata(const string& path) -> array<string, PROPERTY_COUNT> {
    array<string, PROPERTY_COUNT> metadata;
    AVFormatContext* formatContext = nullptr;

    if (avformat_open_input(&formatContext, path.c_str(), nullptr, nullptr) <
        0) {
        return metadata;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        return metadata;
    }

    AVDictionary* tags = formatContext->metadata;
    auto getTag = [&](const char* key) -> string {
        const AVDictionaryEntry* tag = av_dict_get(tags, key, nullptr, 0);
        return tag ? tag->value : "";
    };

    metadata[Path] = path;

    const string titleTag = getTag("title");
    fs::path realPath;

    if (titleTag.empty()) {
        realPath = path;
    }

    metadata[Title] =
        titleTag.empty() ? realPath.filename().string() : titleTag;
    metadata[Artist] = getTag("artist");
    metadata[Album] = getTag("album");
    metadata[TrackNumber] = getTag("track");
    metadata[AlbumArtist] = getTag("album_artist");
    metadata[Genre] = getTag("genre");
    metadata[Year] = getTag("date");
    metadata[Composer] = getTag("composer");
    metadata[BPM] = getTag("TBPM");
    metadata[Language] = getTag("language");
    metadata[DiscNumber] = getTag("disc");
    metadata[Comment] = getTag("comment");
    metadata[Publisher] = getTag("publisher");

    metadata[Duration] = toMinutes(formatContext->duration / AV_TIME_BASE);
    metadata[Bitrate] = roundBitrate(formatContext->bit_rate);

    string cover;

    bool foundAudio = false;
    bool foundCover = false;

    const u8 streams = formatContext->nb_streams;

    for (u8 i = 0; i < streams; i++) {
        const AVStream* stream = formatContext->streams[i];

        if (!foundCover &&
            (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
            cover.resize(stream->attached_pic.size);
            cover = reinterpret_cast<const char*>(stream->attached_pic.data);
            foundCover = true;
        }

        if (!foundAudio && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata[SampleRate] = to_string(stream->codecpar->sample_rate);
            metadata[Channels] =
                to_string(stream->codecpar->ch_layout.nb_channels);

            string formatName = formatContext->iformat->name;

            ranges::transform(formatName, formatName.begin(), [](const u8 chr) {
                return std::toupper(chr);
            });

            metadata[FileFormat] = formatName;
            foundAudio = true;
        }

        if (foundAudio && foundCover) {
            break;
        }
    }

    metadata[Cover] = cover;

    avformat_close_input(&formatContext);
    return metadata;
}