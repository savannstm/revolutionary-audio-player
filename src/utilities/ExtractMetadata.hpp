#pragma once

#include "Aliases.hpp"
#include "Constants.hpp"
#include "DurationConversions.hpp"
#include "Enums.hpp"
#include "FFMpeg.hpp"
#include "RapidHasher.hpp"
#include "TrackTree.hpp"

#include <QDir>
#include <QFileInfo>

struct CueInfo {
    HashMap<TrackProperty, QString> metadata;
    QList<CUETrack> tracks;
    QString cueFilePath;
};

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

//? FIXME: Is it really needed?
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

    for (const auto* bpmTag : BPM_TAGS) {
        const auto tag = getTag(bpmTag);

        if (!tag.isEmpty()) {
            metadata.insert_or_assign(TrackProperty::BPM, tag);
            break;
        }
    }
    metadata.insert_or_assign(TrackProperty::BPM, "");

    metadata.insert_or_assign(TrackProperty::Language, getTag("language"));
    metadata.insert_or_assign(TrackProperty::DiscNumber, getTag("disc"));
    metadata.insert_or_assign(TrackProperty::Comment, getTag("comment"));
    metadata.insert_or_assign(TrackProperty::Publisher, getTag("publisher"));
    metadata.insert_or_assign(
        TrackProperty::Duration,
        secsToMins(formatContext->duration / AV_TIME_BASE)
    );
    metadata.insert_or_assign(
        TrackProperty::Bitrate,
        roundBitrate(formatContext->bit_rate)
    );

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

template <usize N>
constexpr auto sliceValue(const QString& line, const char (&key)[N])
    -> QString {
    constexpr u32 keyLen = N - 1;
    return line.sliced(keyLen + 1, line.size() - keyLen - 2);
}

inline auto parseCUE(QFile& cueFile, const QFileInfo& fileInfo) -> CueInfo {
    QTextStream input = QTextStream(&cueFile);

    bool listingTracks = false;
    u16 offset = 0;

    std::optional<CUETrack> track;
    QList<CUETrack> tracks;

    HashMap<TrackProperty, QString> metadata;

    auto addTrack = [&tracks, &track] {
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
                    metadata[TrackProperty::Format] + "/CUE"
                );
            }
        }
    }

    addTrack();

    return { .metadata = metadata,
             .tracks = tracks,
             .cueFilePath = fileInfo.filePath() };
}