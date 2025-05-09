#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"
#include "ffmpeg.hpp"
#include "rapidhasher.hpp"
#include "tominutes.hpp"

using namespace FFmpeg;

inline auto roundBitrate(const u32 bitrate) -> QString {
    const u32 kbps = bitrate / KB_BYTES;

    u16 closest = STANDARD_BITRATES[0];
    u32 minDiff = abs(as<i32>(kbps - closest));

    for (const u16 bitrate : STANDARD_BITRATES) {
        const u32 diff = abs(as<i32>(kbps - bitrate));

        if (diff < minDiff) {
            minDiff = diff;
            closest = bitrate;
        }
    }

    return u"%1k"_s.arg(closest);
}

inline auto extractMetadata(cstr filePath) -> MetadataMap {
    MetadataMap metadata;

    FormatContextPtr formatContext;
    AVFormatContext* formatContextPtr = formatContext.get();

    if (avformat_open_input(&formatContextPtr, filePath, nullptr, nullptr) <
        0) {
        return metadata;
    }

    formatContext.reset(formatContextPtr);

    if (avformat_find_stream_info(formatContextPtr, nullptr) < 0) {
        return metadata;
    }

    AVDictionary* tags = formatContext->metadata;
    const auto getTag = [&](cstr key) -> QString {
        const AVDictionaryEntry* tag = av_dict_get(tags, key, nullptr, 0);
        return tag ? tag->value : QString();
    };

    metadata.emplace(Path, filePath);

    const QString titleTag = getTag("title");
    path realPath;

    if (titleTag.isEmpty()) {
        realPath = filePath;
    }

    metadata.emplace(
        Title,
        titleTag.isEmpty() ? realPath.filename().string().c_str() : titleTag
    );
    metadata.emplace(Artist, getTag("artist"));
    metadata.emplace(Album, getTag("album"));
    metadata.emplace(TrackNumber, getTag("track"));
    metadata.emplace(AlbumArtist, getTag("album_artist"));
    metadata.emplace(Genre, getTag("genre"));
    metadata.emplace(Year, getTag("date"));
    metadata.emplace(Composer, getTag("composer"));
    metadata.emplace(BPM, getTag("TBPM"));
    metadata.emplace(Language, getTag("language"));
    metadata.emplace(DiscNumber, getTag("disc"));
    metadata.emplace(Comment, getTag("comment"));
    metadata.emplace(Publisher, getTag("publisher"));

    metadata.emplace(
        Duration,
        toMinutes(formatContext->duration / AV_TIME_BASE)
    );
    metadata.emplace(Bitrate, roundBitrate(formatContext->bit_rate));

    const u8 streams = formatContext->nb_streams;
    for (u8 i = 0; i < streams; i++) {
        const AVStream* stream = formatContext->streams[i];

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata.emplace(
                SampleRate,
                QString::number(stream->codecpar->sample_rate)
            );
            metadata.emplace(
                Channels,
                QString::number(stream->codecpar->ch_layout.nb_channels)
            );

            QString formatName = formatContext->iformat->name;
            metadata.emplace(Format, formatName.toUpper());
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