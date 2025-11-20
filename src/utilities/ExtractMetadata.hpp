#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "DurationConversions.hpp"
#include "Enums.hpp"
#include "FFMpeg.hpp"
#include "TrackTree.hpp"

#include <QDir>
#include <QFileInfo>

struct CueInfo {
    TrackMetadata metadata;
    QList<CUETrack> tracks;
    QString cueFilePath;
};

using namespace FFmpeg;

inline auto
FFmpegError(const cstr file, const i32 line, const cstr func, const i32 err)
    -> QString {
    array<char, AV_ERROR_MAX_STRING_SIZE> buffer;
    av_strerror(err, buffer.data(), buffer.size());

    QString string;
    string.reserve(AV_ERROR_MAX_STRING_SIZE * 4);

    auto stream = QTextStream(&string);

    stream << file << ' ' << line << ' ' << func << ' ' << buffer.data();

    return string;
}

#define FFMPEG_ERROR(err) FFmpegError(__FILE__, __LINE__, __func__, err)

inline auto extractMetadata(const QString& filePath)
    -> result<TrackMetadata, QString> {
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

    const auto getTag = [&formatContext](cstr key) -> QString {
        const AVDictionaryEntry* tag =
            av_dict_get(formatContext->metadata, key, nullptr, 0);
        return tag ? tag->value : QString();
    };

    TrackMetadata metadata;
    metadata.insert_or_assign(TrackProperty::Path, filePath);

    const QString titleTag = getTag("title");

    if (titleTag.isEmpty()) {
        isize lastIndex = filePath.lastIndexOf('/');

        if (lastIndex == -1) {
            lastIndex = filePath.lastIndexOf('\\');
        }

        metadata.insert_or_assign(
            TrackProperty::Title,
            filePath.sliced(lastIndex + 1)
        );
    } else {
        metadata.insert_or_assign(TrackProperty::Title, titleTag);
    }

    metadata.insert_or_assign(TrackProperty::Artist, getTag("artist"));
    metadata.insert_or_assign(TrackProperty::Album, getTag("album"));
    metadata.insert_or_assign(TrackProperty::TrackNumber, getTag("track"));
    metadata.insert_or_assign(
        TrackProperty::AlbumArtist,
        getTag("album_artist")
    );
    metadata.insert_or_assign(TrackProperty::Genre, getTag("genre"));
    metadata.insert_or_assign(TrackProperty::Year, getTag("date"));
    metadata.insert_or_assign(TrackProperty::Composer, getTag("composer"));

    for (const auto* const bpmTag : BPM_TAGS) {
        const QString tag = getTag(bpmTag);

        if (!tag.isEmpty()) {
            metadata.insert_or_assign(TrackProperty::BPM, tag);
            break;
        }
    }

    metadata.emplace(TrackProperty::BPM, "");

    metadata.insert_or_assign(TrackProperty::Language, getTag("language"));
    metadata.insert_or_assign(TrackProperty::DiscNumber, getTag("disc"));
    metadata.insert_or_assign(TrackProperty::Comment, getTag("comment"));
    metadata.insert_or_assign(TrackProperty::Publisher, getTag("publisher"));
    metadata.insert_or_assign(
        TrackProperty::Duration,
        secsToMins(formatContext->duration / AV_TIME_BASE)
    );

    // First get nominal bitrate of the codec
    u32 bitrate = audioStream->codecpar->bit_rate;

    // If codec doesn't specify nominal bitrate or if it's loseless, get it from
    // format context
    if (bitrate == 0) {
        bitrate = formatContext->bit_rate;
    }

    bitrate /= KB_BYTES;

    QString bitrateString = QString::number(bitrate);
    bitrateString += u"k";

    metadata.insert_or_assign(TrackProperty::Bitrate, bitrateString);

    metadata.insert_or_assign(
        TrackProperty::SampleRate,
        QString::number(audioStream->codecpar->sample_rate)
    );

    metadata.insert_or_assign(
        TrackProperty::Channels,
        QString::number(audioStream->codecpar->ch_layout.nb_channels)
    );

    const QString formatName = formatContext->iformat->name;
    metadata.insert_or_assign(TrackProperty::Format, formatName.toUpper());

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

    const u8 attachmentStreamIndex = u8(errorCode);
    const AVStream* const attachmentStream =
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

template <usize N>
constexpr auto sliceValue(const QString& line, const char (&key)[N])
    -> QString {
    constexpr u32 keyLen = N - 1;
    return line.sliced(keyLen + 1, line.size() - keyLen - 2);
}

inline auto parseCUE(QFile& cueFile, const QFileInfo& fileInfo) -> CueInfo {
    auto input = QTextStream(&cueFile);

    bool listingTracks = false;
    u16 offset = 0;

    optional<CUETrack> track;
    QList<CUETrack> tracks;

    TrackMetadata metadata;

    auto addTrack = [&tracks, &track] -> void {
        if (track) {
            tracks.emplace_back(std::move(*track));
            track.reset();
        }
    };

    while (!input.atEnd()) {
        const QString line = input.readLine().trimmed();

        if (listingTracks) {
            if (line.startsWith(u"TRACK"_qsv)) {
                addTrack();
                track =
                    CUETrack{ .trackNumber = line.sliced(sizeof("TRACK"), 2) };
            } else if (line.startsWith(u"TITLE"_qsv)) {
                track->title = sliceValue(line, "TITLE ");
            } else if (line.startsWith(u"PERFORMER"_qsv)) {
                track->artist = sliceValue(line, "PERFORMER ");
            } else if (line.startsWith(u"INDEX 01"_qsv)) {
                track->offset = timeToSecs(line.sliced(sizeof("INDEX 01"), 5));
            }
        } else {
            if (line.startsWith(u"PERFORMER"_qsv)) {
                metadata.insert_or_assign(
                    TrackProperty::Artist,
                    sliceValue(line, "PERFORMER ")
                );
            } else if (line.startsWith(u"TITLE"_qsv)) {
                metadata.insert_or_assign(
                    TrackProperty::Album,
                    sliceValue(line, "TITLE ")
                );
            } else if (line.startsWith(u"REM DATE"_qsv)) {
                metadata.insert_or_assign(
                    TrackProperty::Year,
                    line.sliced(sizeof("REM DATE"))
                );
            } else if (line.startsWith(u"REM GENRE"_qsv)) {
                metadata.insert_or_assign(
                    TrackProperty::Genre,
                    sliceValue(line, "REM GENRE ")
                );
            } else if (line.startsWith(u"FILE"_qsv)) {
                listingTracks = true;

                const QString filename = line.sliced(
                    line.indexOf('"') + 1,
                    line.lastIndexOf('"') - line.indexOf('"') - 1
                );
                const QString path = fileInfo.dir().filePath(filename);

                auto extracted = extractMetadata(path).value();
                extracted.insert_or_assign(TrackProperty::Path, path);

                for (const auto& [key, value] : metadata) {
                    extracted.insert_or_assign(key, value);
                }

                metadata = std::move(extracted);
                metadata.insert_or_assign(
                    TrackProperty::Format,
                    metadata[TrackProperty::Format] + u"/CUE"_qssv
                );
            }
        }
    }

    addTrack();

    return { .metadata = metadata,
             .tracks = tracks,
             .cueFilePath = fileInfo.filePath() };
}