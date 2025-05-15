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

#include <QDebug>

using namespace FFmpeg;

// Maximum, is, I think, 510 kbps in Vorbis encoding, but ChatGPT said it can go
// up to 530 kbps in AAC. So I just added this offset, just to be save.
constexpr u16 MAX_LOSSY_BITRATE = 550;

inline auto roundBitrate(const u32 bitrate) -> QString {
    const u32 kbps = bitrate / KB_BYTES;

    if (kbps > MAX_LOSSY_BITRATE) {
        return u"%1k"_s.arg(kbps);
    }

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

inline auto extractMetadata(const QString& filePath)
    -> HashMap<TrackProperty, QString> {
    HashMap<TrackProperty, QString> metadata;

    FormatContextPtr formatContext;
    AVFormatContext* formatContextPtr = formatContext.get();

    i32 err;
    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    err = avformat_open_input(
        &formatContextPtr,
        filePath.toStdString().c_str(),
        nullptr,
        nullptr
    );
    if (err < 0) {
        qWarning() << av_make_error_string(
            errBuf.data(),
            AV_ERROR_MAX_STRING_SIZE,
            err
        );
        return metadata;
    }

    formatContext.reset(formatContextPtr);

    err = avformat_find_stream_info(formatContextPtr, nullptr);
    if (err < 0) {
        qWarning() << av_make_error_string(
            errBuf.data(),
            AV_ERROR_MAX_STRING_SIZE,
            err
        );
        return metadata;
    }

    AVDictionary* tags = formatContext->metadata;
    const auto getTag = [&](cstr key) -> QString {
        const AVDictionaryEntry* tag = av_dict_get(tags, key, nullptr, 0);
        return tag ? tag->value : QString();
    };

    metadata.emplace(Path, filePath);

    const QString titleTag = getTag("title");

    if (titleTag.isEmpty()) {
        isize lastIndex = filePath.lastIndexOf('/');

        if (lastIndex == -1) {
            lastIndex = filePath.lastIndexOf('\\');
        }

        metadata.emplace(Title, filePath.sliced(lastIndex));
    } else {
        metadata.emplace(Title, titleTag);
    }

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

    const u8 streamCount = formatContext->nb_streams;
    const span<AVStream*> streams = span(formatContext->streams, streamCount);

    for (const AVStream* stream : streams) {
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            metadata.emplace(
                SampleRate,
                QString::number(stream->codecpar->sample_rate)
            );
            metadata.emplace(
                Channels,
                QString::number(stream->codecpar->ch_layout.nb_channels)
            );

            const QString formatName = formatContext->iformat->name;
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

    i32 err;
    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    err = avformat_open_input(&formatContextPtr, path, nullptr, nullptr);
    if (err < 0) {
        qWarning() << av_make_error_string(
            errBuf.data(),
            AV_ERROR_MAX_STRING_SIZE,
            err
        );
        return cover;
    }

    formatContext.reset(formatContextPtr);
    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        qWarning() << av_make_error_string(
            errBuf.data(),
            AV_ERROR_MAX_STRING_SIZE,
            err
        );
        return cover;
    }

    const u8 streamCount = formatContext->nb_streams;
    const span<AVStream*> streams = span(formatContext->streams, streamCount);

    for (const AVStream* stream : streams) {
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