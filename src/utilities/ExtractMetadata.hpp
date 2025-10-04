#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "DurationConversions.hpp"
#include "Enums.hpp"
#include "FFMpeg.hpp"
#include "RapidHasher.hpp"

using namespace FFmpeg;

constexpr array<cstr, 3> BPM_TAGS = { "BPM", "TBPM", "IBPM" };

inline auto
FFmpegError(const cstr file, const i32 line, const cstr func, const i32 err)
    -> QString {
    array<char, AV_ERROR_MAX_STRING_SIZE> buffer;
    av_strerror(err, buffer.data(), buffer.size());

    QString string;
    string.reserve(AV_ERROR_MAX_STRING_SIZE * 4);

    QTextStream stream = QTextStream(&string);

    stream << file << ' ' << line << ' ' << func << ' ' << buffer.data();

    return string;
}

#define FFMPEG_ERROR(err) FFmpegError(__FILE__, __LINE__, __func__, err)

inline auto roundBitrate(const u32 bitrate) -> QString {
    const u32 kbps = bitrate / KB_BYTES;

    if (kbps < MIN_LOSSY_BITRATE || kbps > MAX_LOSSY_BITRATE) {
        return u"%1k"_s.arg(kbps);
    }

    u16 closest = STANDARD_BITRATES[0];
    u32 minDiff = abs(i32(kbps - closest));

    for (const u16 bitrate : STANDARD_BITRATES) {
        const u32 diff = abs(i32(kbps - bitrate));

        if (diff < minDiff) {
            minDiff = diff;
            closest = bitrate;
        }
    }

    return u"%1k"_s.arg(closest);
}

inline auto extractMetadata(const QString& filePath)
    -> result<HashMap<TrackProperty, QString>, QString> {
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
        return err(FFMPEG_ERROR(errorCode));
    }

    formatContext.reset(fCtxPtr);

    errorCode = avformat_find_stream_info(fCtxPtr, nullptr);
    if (errorCode < 0) {
        return err(FFMPEG_ERROR(errorCode));
    }

    errorCode =
        av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (errorCode < 0) {
        return err(FFMPEG_ERROR(errorCode));
    }

    const i8 audioStreamIndex = i8(errorCode);
    const AVStream* audioStream = formatContext->streams[audioStreamIndex];

    const auto getTag = [&](cstr key) -> QString {
        const AVDictionaryEntry* tag =
            av_dict_get(formatContext->metadata, key, nullptr, 0);
        return tag ? tag->value : QString();
    };

    HashMap<TrackProperty, QString> metadata;
    metadata.emplace(TrackProperty::Path, filePath);

    const QString titleTag = getTag("title");

    if (titleTag.isEmpty()) {
        isize lastIndex = filePath.lastIndexOf('/');

        if (lastIndex == -1) {
            lastIndex = filePath.lastIndexOf('\\');
        }

        metadata.emplace(TrackProperty::Title, filePath.sliced(lastIndex + 1));
    } else {
        metadata.emplace(TrackProperty::Title, titleTag);
    }

    metadata.emplace(TrackProperty::Artist, getTag("artist"));
    metadata.emplace(TrackProperty::Album, getTag("album"));
    metadata.emplace(TrackProperty::TrackNumber, getTag("track"));
    metadata.emplace(TrackProperty::AlbumArtist, getTag("album_artist"));
    metadata.emplace(TrackProperty::Genre, getTag("genre"));
    metadata.emplace(TrackProperty::Year, getTag("date"));
    metadata.emplace(TrackProperty::Composer, getTag("composer"));

    for (const auto* bpmTag : BPM_TAGS) {
        const auto tag = getTag(bpmTag);

        if (!tag.isEmpty()) {
            metadata.emplace(TrackProperty::BPM, tag);
            break;
        }
    }
    metadata.emplace(TrackProperty::BPM, "");

    metadata.emplace(TrackProperty::Language, getTag("language"));
    metadata.emplace(TrackProperty::DiscNumber, getTag("disc"));
    metadata.emplace(TrackProperty::Comment, getTag("comment"));
    metadata.emplace(TrackProperty::Publisher, getTag("publisher"));
    metadata.emplace(
        TrackProperty::Duration,
        secsToMins(formatContext->duration / AV_TIME_BASE)
    );
    metadata.emplace(
        TrackProperty::Bitrate,
        roundBitrate(formatContext->bit_rate)
    );

    metadata.emplace(
        TrackProperty::SampleRate,
        QString::number(audioStream->codecpar->sample_rate)
    );

    metadata.emplace(
        TrackProperty::Channels,
        QString::number(audioStream->codecpar->ch_layout.nb_channels)
    );

    const QString formatName = formatContext->iformat->name;
    metadata.emplace(TrackProperty::Format, formatName.toUpper());

    return metadata;
}

inline auto extractCover(cstr path) -> result<vector<u8>, QString> {
    FormatContext formatContext;
    AVFormatContext* fCtxPtr = formatContext.get();

    vector<u8> coverBytes;
    i32 errorCode;

    errorCode = avformat_open_input(&fCtxPtr, path, nullptr, nullptr);
    if (errorCode < 0) {
        return err(FFMPEG_ERROR(errorCode));
    }

    formatContext.reset(fCtxPtr);
    errorCode = avformat_find_stream_info(fCtxPtr, nullptr);

    if (errorCode < 0) {
        return err(FFMPEG_ERROR(errorCode));
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
            return err(FFMPEG_ERROR(errorCode));
        }
    }

    const i8 attachmentStreamIndex = i8(errorCode);
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