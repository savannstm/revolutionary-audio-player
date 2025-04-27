#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

#include "aliases.hpp"
#include "ffmpeg.hpp"
#include "tominutes.hpp"

#include <QMap>

constexpr array<u16, 18> STANDARD_BITRATES = { 32,  40,  48,  56,  64,  80,
                                               96,  112, 128, 160, 192, 224,
                                               256, 320, 350, 384, 448, 510 };

using namespace FFmpeg;

inline auto roundBitrate(const u32 bitrate) -> QString {
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

    return u"%1k"_s.arg(closest);
}

inline auto extractMetadata(cstr path) -> QMap<u8, QString> {
    QMap<u8, QString> metadata;

    FormatContextPtr formatContext;
    AVFormatContext* formatContextPtr = formatContext.get();

    if (avformat_open_input(&formatContextPtr, path, nullptr, nullptr) < 0) {
        return metadata;
    }

    formatContext.reset(formatContextPtr);

    if (avformat_find_stream_info(formatContextPtr, nullptr) < 0) {
        return metadata;
    }

    AVDictionary* tags = formatContext->metadata;
    auto getTag = [&](cstr key) -> QString {
        const AVDictionaryEntry* tag = av_dict_get(tags, key, nullptr, 0);
        return tag ? tag->value : "";
    };

    metadata[Path] = path;

    const QString titleTag = getTag("title");
    fs::path realPath;

    if (titleTag.isEmpty()) {
        realPath = path;
    }

    metadata[Title] =
        titleTag.isEmpty() ? realPath.filename().string().c_str() : titleTag;
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

    bool foundAudio = false;

    const u8 streams = formatContext->nb_streams;

    for (u8 i = 0; i < streams; i++) {
        const AVStream* stream = formatContext->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata[SampleRate] =
                QString::number(stream->codecpar->sample_rate);
            metadata[Channels] =
                QString::number(stream->codecpar->ch_layout.nb_channels);

            QString formatName = formatContext->iformat->name;
            metadata[Format] = formatName.toUpper();
            break;
        }
    }

    return metadata;
}

inline auto extractCover(cstr path) -> vector<u8> {
    FormatContextPtr formatContext;
    AVFormatContext* formatContextPtr = formatContext.get();

    vector<u8> cover;

    if (avformat_open_input(&formatContextPtr, path, nullptr, nullptr) < 0) {
        return cover;
    }

    formatContext.reset(formatContextPtr);

    if (avformat_find_stream_info(formatContextPtr, nullptr) < 0) {
        return cover;
    }

    const u8 streams = formatContext->nb_streams;
    for (u8 i = 0; i < streams; i++) {
        const AVStream* stream = formatContext->streams[i];

        if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
            cover.resize(stream->attached_pic.size);
            memcpy(
                cover.data(),
                stream->attached_pic.data,
                stream->attached_pic.size
            );
            break;
        }
    }

    return cover;
}