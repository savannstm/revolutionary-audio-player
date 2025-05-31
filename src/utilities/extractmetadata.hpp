#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "enums.hpp"
#include "ffmpeg.hpp"
#include "rapidhasher.hpp"
#include "tominutes.hpp"

#include <QDebug>

using namespace FFmpeg;

inline auto FFmpegError(const i32 err) -> string {
    string buffer(AV_ERROR_MAX_STRING_SIZE, '\0');
    av_strerror(err, buffer.data(), buffer.size());
    return buffer;
}

inline auto roundBitrate(const u32 bitrate) -> QString {
    const u32 kbps = bitrate / KB_BYTES;

    if (kbps < MIN_LOSSY_BITRATE || kbps > MAX_LOSSY_BITRATE) {
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
    -> result<HashMap<TrackProperty, QString>, string> {
    FormatContext formatContext;
    AVFormatContext* fCtxPtr = formatContext.get();

    i32 errorCode;

    errorCode = avformat_open_input(
        &fCtxPtr,
        filePath.toStdString().c_str(),
        nullptr,
        nullptr
    );
    if (errorCode < 0) {
        return err(FFmpegError(errorCode));
    }

    formatContext.reset(fCtxPtr);

    errorCode = avformat_find_stream_info(fCtxPtr, nullptr);
    if (errorCode < 0) {
        return err(FFmpegError(errorCode));
    }

    errorCode =
        av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (errorCode < 0) {
        return err(FFmpegError(errorCode));
    }

    const i8 audioStreamIndex = as<i8>(errorCode);
    const AVStream* audioStream = formatContext->streams[audioStreamIndex];

    const auto getTag = [&](cstr key) -> QString {
        const AVDictionaryEntry* tag =
            av_dict_get(formatContext->metadata, key, nullptr, 0);
        return tag ? tag->value : QString();
    };

    HashMap<TrackProperty, QString> metadata;
    metadata.emplace(Path, filePath);

    const QString titleTag = getTag("title");

    if (titleTag.isEmpty()) {
        isize lastIndex = filePath.lastIndexOf('/');

        if (lastIndex == -1) {
            lastIndex = filePath.lastIndexOf('\\');
        }

        metadata.emplace(Title, filePath.sliced(lastIndex + 1));
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

    metadata.emplace(
        SampleRate,
        QString::number(audioStream->codecpar->sample_rate)
    );

    metadata.emplace(
        Channels,
        QString::number(audioStream->codecpar->ch_layout.nb_channels)
    );

    const QString formatName = formatContext->iformat->name;
    metadata.emplace(Format, formatName.toUpper());

    return metadata;
}

inline auto extractCover(cstr path) -> result<vector<u8>, string> {
    FormatContext formatContext;
    AVFormatContext* fCtxPtr = formatContext.get();

    vector<u8> coverBytes;
    i32 errorCode;

    errorCode = avformat_open_input(&fCtxPtr, path, nullptr, nullptr);
    if (errorCode < 0) {
        return err(FFmpegError(errorCode));
    }

    formatContext.reset(fCtxPtr);
    errorCode = avformat_find_stream_info(fCtxPtr, nullptr);

    if (errorCode < 0) {
        return err(FFmpegError(errorCode));
    }

    errorCode =
        av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (errorCode < 0) {
        errorCode = av_find_best_stream(
            fCtxPtr,
            AVMEDIA_TYPE_ATTACHMENT,
            -1,
            -1,
            nullptr,
            0
        );

        if (errorCode < 0) {
            return err(FFmpegError(errorCode));
        }
    }

    const i8 attachmentStreamIndex = as<i8>(errorCode);
    const AVStream* attachmentStream =
        formatContext->streams[attachmentStreamIndex];

    if (attachmentStream->attached_pic.size == 0) {
        return err("Unable to find cover attachment.");
    }

    coverBytes.resize(attachmentStream->attached_pic.size);
    memcpy(
        coverBytes.data(),
        attachmentStream->attached_pic.data,
        attachmentStream->attached_pic.size
    );

    return coverBytes;
}