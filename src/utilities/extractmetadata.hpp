#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

#include "aliases.hpp"
#include "constants.hpp"
#include "tominutes.hpp"

#include <algorithm>

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

auto extractMetadata(const string& path) -> metadata_array {
    metadata_array metadata;
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
    auto getTag = [&](cstr key) -> string {
        const AVDictionaryEntry* tag = av_dict_get(tags, key, nullptr, 0);
        return tag ? tag->value : "";
    };

    metadata[TrackProperty::Path] = path;

    const string titleTag = getTag("title");
    fs::path realPath;

    if (titleTag.empty()) {
        realPath = path;
    }

    metadata[TrackProperty::Title] =
        titleTag.empty() ? realPath.filename().string() : titleTag;
    metadata[TrackProperty::Artist] = getTag("artist");
    metadata[TrackProperty::Album] = getTag("album");
    metadata[TrackProperty::TrackNumber] = getTag("track");
    metadata[TrackProperty::AlbumArtist] = getTag("album_artist");
    metadata[TrackProperty::Genre] = getTag("genre");
    metadata[TrackProperty::Year] = getTag("date");
    metadata[TrackProperty::Composer] = getTag("composer");
    metadata[TrackProperty::BPM] = getTag("TBPM");
    metadata[TrackProperty::Language] = getTag("language");
    metadata[TrackProperty::DiscNumber] = getTag("disc");
    metadata[TrackProperty::Comment] = getTag("comment");
    metadata[TrackProperty::Publisher] = getTag("publisher");

    metadata[TrackProperty::Duration] =
        toMinutes(formatContext->duration / AV_TIME_BASE);
    metadata[TrackProperty::Bitrate] = roundBitrate(formatContext->bit_rate);

    string cover;

    bool foundAudio = false;
    bool foundCover = false;

    const u8 streams = formatContext->nb_streams;

    for (u8 i = 0; i < streams; i++) {
        const AVStream* stream = formatContext->streams[i];

        if (!foundCover &&
            (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
            // TODO: Fetch cover on demand, don't decode it here
            cover = string(
                reinterpret_cast<cstr>(stream->attached_pic.data),
                stream->attached_pic.size
            );
            foundCover = true;
        }

        if (!foundAudio && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata[TrackProperty::SampleRate] =
                to_string(stream->codecpar->sample_rate);
            metadata[TrackProperty::Channels] =
                to_string(stream->codecpar->ch_layout.nb_channels);

            string formatName = formatContext->iformat->name;

            ranges::transform(formatName, formatName.begin(), [](const u8 chr) {
                return toupper(chr);
            });

            metadata[TrackProperty::Format] = formatName;
            foundAudio = true;
        }

        if (foundAudio && foundCover) {
            break;
        }
    }

    metadata[TrackProperty::Cover] = cover;

    // TODO: Don't close the input to reuse?
    avformat_close_input(&formatContext);
    return metadata;
}