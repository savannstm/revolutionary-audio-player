// NOLINT
#include "FFMpeg.hpp"
// NOLINTEND

#include "Constants.hpp"
#include "Duration.hpp"
#include "Enums.hpp"
#include "Utils.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QTextStream>
#include <numbers>

using namespace FFmpeg;

auto trackPropertyLabel(const TrackProperty property) -> QString {
    return QObject::tr(TRACK_PROPERTIES_INFOS[u8(property)].label);
}

// NOLINTBEGIN: xorshift32

auto randint() -> u32 {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

auto randint_range(const u32 min, const u32 max) -> u32 {
    const u32 range = max - min + 1;
    const u32 limit = UINT32_MAX - (UINT32_MAX % range);

    u32 x;

    do {
        x = randint();
    } while (x >= limit);

    return min + (x % range);
}

// NOLINTEND

auto parsePreset(const QByteArray& presetData)
    -> result<EqualizerPreset, QString> {
    EqualizerPreset preset{};

    auto textStream =
        QTextStream(presetData, QTextStream::ReadOnly | QTextStream::Text);
    textStream.setEncoding(QStringConverter::Utf8);

    QString bandCountString = QString::number(u8(preset.bandCount));
    textStream >> preset.name;

    textStream.seek(textStream.pos() + 1);
    textStream >> bandCountString;

    for (const u8 band : range<u8>(0, u8(preset.bandCount))) {
        QString bandString = QString::number(preset.bands[band]);
        textStream.seek(textStream.pos() + 1);
        textStream >> bandString;
    }

    if (textStream.status() != QTextStream::Ok) {
        return Err(u"Unable to parse preset. Error %1 occurred."_s.arg(
            textStream.status()
        ));
    }

    return preset;
}

auto serializePreset(const EqualizerPreset& preset) -> QByteArray {
    QByteArray presetData;
    presetData.reserve(sizeof(EqualizerPreset));

    auto textStream =
        QTextStream(&presetData, QTextStream::WriteOnly | QTextStream::Text);
    textStream.setEncoding(QStringConverter::Utf8);

    textStream << preset.name << '\n' << u8(preset.bandCount) << '\n';

    for (const i8 gain : span(preset.bands.data(), u8(preset.bandCount))) {
        textStream << gain << '\n';
    }

    return presetData;
}

auto exportXSPF(
    const QString& outputPath,
    const vector<TrackMetadata>& metadataVec
) -> result<bool, QString> {
    auto file = QFile(outputPath);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return Err(file.errorString());
    }

    auto output = QTextStream(&file);
    output.setEncoding(QStringConverter::Utf8);

    output << R"(<?xml version="1.0" encoding="UTF-8"?>)";
    output << '\n';
    output << R"(<playlist version="1" xmlns="http://xspf.org/ns/0/">)";
    output << '\n';
    output << "<trackList>";
    output << '\n';

    for (const auto& metadata : metadataVec) {
        output << "<track>";
        output << '\n';

        for (const auto& [property, value] : metadata) {
            cstr tag = TRACK_PROPERTIES_INFOS[u8(property)].xspfTag;

            if (tag == nullptr) {
                break;
            }

            QString content = value;

            if (property == TrackProperty::Path) {
                content =
                    QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                        .relativeFilePath(value);
            }

            output << '<' << tag << '>' << content.toHtmlEscaped() << "</"
                   << tag << '>';
            output << '\n';
        }

        output << "</track>";
        output << '\n';
    }

    output << "</trackList>";
    output << '\n';
    output << "</playlist>";
    output << '\n';

    return true;
}

auto exportM3U8(
    const QString& outputPath,
    const vector<TrackMetadata>& metadataVector
) -> result<bool, QString> {
    auto outputFile = QFile(outputPath);

    if (!outputFile.open(QFile::WriteOnly | QFile::Text)) {
        return Err(outputFile.errorString());
    }

    auto output = QTextStream(&outputFile);
    output.setEncoding(QStringConverter::Utf8);

    output << "#EXTM3U";
    output << '\n';

    for (const auto& metadata : metadataVector) {
        const QString title = metadata[TrackProperty::Title];
        const QString artist = metadata[TrackProperty::Artist];
        const QString path = metadata[TrackProperty::Path];
        const u32 durationSecs = metadata[TrackProperty::Duration].toUInt();

        output << "#EXTINF:";
        output << durationSecs << ',' << artist << " - " << title;
        output << '\n';

        output << QDir(outputPath.sliced(0, outputPath.lastIndexOf('/')))
                      .relativeFilePath(path);
        output << '\n';
    }

    return true;
}

template <usize N>
constexpr auto sliceValue(const QString& line, const char (&key)[N])
    -> QString {
    constexpr u32 keyLen = N - 1;
    return line.sliced(keyLen + 1, line.size() - keyLen - 2);
}

auto parseCUE(QFile& cueFile, const QFileInfo& fileInfo) -> CUEInfo {
    auto input = QTextStream(&cueFile);

    bool listingTracks = false;
    u32 offset = 0;

    optional<CUETrack> track;
    vector<CUETrack> tracks;

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
            if (line.startsWith(u"TRACK")) {
                addTrack();
                track =
                    CUETrack{ .trackNumber = line.sliced(sizeof("TRACK"), 2) };
            } else if (line.startsWith(u"TITLE")) {
                track->title = sliceValue(line, "TITLE ");
            } else if (line.startsWith(u"PERFORMER")) {
                track->artist = sliceValue(line, "PERFORMER ");
            } else if (line.startsWith(u"INDEX 01")) {
                track->offset = Duration::stringToSeconds(
                                    line.sliced(sizeof("INDEX 01"), 5)
                )
                                    .value();
            }
        } else {
            if (line.startsWith(u"PERFORMER")) {
                metadata.insert_or_assign(
                    TrackProperty::Artist,
                    sliceValue(line, "PERFORMER ")
                );
            } else if (line.startsWith(u"TITLE")) {
                metadata.insert_or_assign(
                    TrackProperty::Album,
                    sliceValue(line, "TITLE ")
                );
            } else if (line.startsWith(u"REM DATE")) {
                metadata.insert_or_assign(
                    TrackProperty::Year,
                    line.sliced(sizeof("REM DATE"))
                );
            } else if (line.startsWith(u"REM GENRE")) {
                metadata.insert_or_assign(
                    TrackProperty::Genre,
                    sliceValue(line, "REM GENRE ")
                );
            } else if (line.startsWith(u"FILE")) {
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
                    TrackProperty::AudioFormat,
                    metadata[TrackProperty::AudioFormat] + u"/CUE"_qssv
                );
            }
        }
    }

    addTrack();

    return { .metadata = metadata,
             .tracks = tracks,
             .cueFilePath = fileInfo.filePath() };
}

auto applyEffectToImage(
    const QImage& src,
    QGraphicsEffect* const effect,
    i32 extent
) -> QImage {
    if (src.isNull()) {
        return {};
    }

    if (effect == nullptr) {
        return src;
    }

    QGraphicsScene scene;

    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(src));
    item.setGraphicsEffect(effect);

    scene.addItem(&item);

    QImage res = QImage(
        src.size() + QSize(extent * 2, extent * 2),
        QImage::Format_ARGB32
    );
    res.fill(Qt::transparent);

    auto painter = QPainter(&res);
    scene.render(
        &painter,
        QRectF(),
        QRectF(
            -extent,
            -extent,
            src.width() + (extent * 2),
            src.height() + (extent * 2)
        )
    );

    return res;
}

auto FFmpegError(
    const cstr file,
    const i32 line,
    const cstr func,
    const i32 err
) -> QString {
    array<char, AV_ERROR_MAX_STRING_SIZE> buffer;
    av_strerror(err, buffer.data(), buffer.size());

    QString string;
    string.reserve(AV_ERROR_MAX_STRING_SIZE * 4);

    auto stream = QTextStream(&string);

    stream << file << ' ' << line << ' ' << func << ' ' << buffer.data();

    return string;
}

auto extractMetadata(const QString& filePath)
    -> result<TrackMetadata, QString> {
    FormatContext formatContext;
    AVFormatContext* fCtxPtr = formatContext.get();

    i32 err;

    err = avformat_open_input(
        &fCtxPtr,
        filePath.toStdString().c_str(),
        nullptr,
        nullptr
    );
    if (err < 0) {
        return Err(FFMPEG_ERROR(err));
    }

    formatContext.reset(fCtxPtr);

    err = avformat_find_stream_info(fCtxPtr, nullptr);
    if (err < 0) {
        return Err(FFMPEG_ERROR(err));
    }

    err = av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (err < 0) {
        return Err(FFMPEG_ERROR(err));
    }

    const AVStream* const audioStream = formatContext->streams[i8(err)];

    TrackMetadata metadata;
    metadata.reserve(TRACK_PROPERTY_COUNT);

    for (const auto prop : TRACK_PROPERTIES) {
        switch (prop) {
            case TrackProperty::Bitrate: {
                // First get nominal bitrate of the
                // codec
                u32 bitrate = audioStream->codecpar->bit_rate;

                // If codec doesn't specify nominal bitrate or if it's loseless,
                // get it from format context
                if (bitrate == 0) {
                    bitrate = formatContext->bit_rate;
                }

                bitrate /= KB_BYTES;

                const QString bitrateString = QString::number(bitrate) + 'k';

                metadata.emplace(TrackProperty::Bitrate, bitrateString);
                break;
            }
            case TrackProperty::SampleRate:
                metadata.emplace(
                    TrackProperty::SampleRate,
                    QString::number(audioStream->codecpar->sample_rate)
                );

                break;
            case TrackProperty::Channels:
                metadata.emplace(
                    TrackProperty::Channels,
                    QString::number(
                        audioStream->codecpar->ch_layout.nb_channels
                    )
                );

                break;
            case TrackProperty::AudioFormat: {
                const QString formatName =
                    avcodec_get_name(audioStream->codecpar->codec_id);
                metadata.emplace(
                    TrackProperty::AudioFormat,
                    formatName.toUpper()
                );
                break;
            }
            case TrackProperty::Path:
                metadata.emplace(TrackProperty::Path, filePath);
                break;
            case TrackProperty::Duration:
                metadata.emplace(
                    TrackProperty::Duration,
                    QString::number(formatContext->duration / AV_TIME_BASE)
                );
                break;
            case TrackProperty::SampleFormat: {
                const auto fmt = AVSampleFormat(audioStream->codecpar->format);
                cstr name = av_get_sample_fmt_name(fmt);

                if (audioStream->codecpar->codec_id == AV_CODEC_ID_PCM_S24LE) {
                    name = "S24";
                }

                metadata.emplace(
                    TrackProperty::SampleFormat,
                    (name != nullptr) ? QString::fromUtf8(name).toUpper()
                                      : QString()
                );
                break;
            }
            default: {
                QString value;

                for (const cstr tag :
                     TRACK_PROPERTIES_INFOS[u8(prop)].ffmpegTags) {
                    if (tag == nullptr) {
                        break;
                    }

                    if (const AVDictionaryEntry* const entry = av_dict_get(
                            formatContext->metadata,
                            tag,
                            nullptr,
                            0
                        )) {
                        value = QString::fromUtf8(entry->value);
                        break;
                    }
                }

                metadata.emplace(prop, value);
                break;
            }
        }
    }

    if (metadata[TrackProperty::Title].isEmpty()) {
        metadata.insert_or_assign(
            TrackProperty::Title,
            QFileInfo(filePath).completeBaseName()
        );
    }

    return metadata;
}

auto extractCover(cstr path) -> result<vector<u8>, QString> {
    FormatContext formatContext;
    AVFormatContext* fCtxPtr = formatContext.get();

    vector<u8> coverBytes;
    i32 errorCode;

    errorCode = avformat_open_input(&fCtxPtr, path, nullptr, nullptr);
    if (errorCode < 0) {
        return Err(FFMPEG_ERROR(errorCode));
    }

    formatContext.reset(fCtxPtr);
    errorCode = avformat_find_stream_info(fCtxPtr, nullptr);

    if (errorCode < 0) {
        return Err(FFMPEG_ERROR(errorCode));
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
            return Err(FFMPEG_ERROR(errorCode));
        }
    }

    const u8 attachmentStreamIndex = u8(errorCode);
    const AVStream* const attachmentStream =
        formatContext->streams[attachmentStreamIndex];

    if (attachmentStream->attached_pic.size == 0) {
        return Err(u"Unable to find cover attachment."_s);
    }

    coverBytes.resize(attachmentStream->attached_pic.size);
    memcpy(
        coverBytes.data(),
        attachmentStream->attached_pic.data,
        attachmentStream->attached_pic.size
    );

    return coverBytes;
}

inline auto sinc(const f64 value) -> f64 {
    if (value == 0.0) {
        return 1.0;
    }

    return std::sin(std::numbers::pi * value) / (std::numbers::pi * value);
}

inline auto blackman(const i32 sampleIdx, const i32 windowLength) -> f64 {
    return 0.42 -
           (0.5 *
            std::cos(2.0 * std::numbers::pi * sampleIdx / (windowLength - 1))) +
           (0.08 *
            std::cos(4.0 * std::numbers::pi * sampleIdx / (windowLength - 1)));
}

void downsample(
    const f32* const input,
    f32* const output,
    const u16 sampleCount,
    const u32 sampleRate
) {
    const f64 resampleRatio = f64(FFT_SAMPLE_RATE) / sampleRate;

    constexpr u8 FILTER_RADIUS = 32;
    constexpr u8 FILTER_SIZE = (FILTER_RADIUS * 2) + 1;
    const f64 cutoff = f64(FFT_SAMPLE_RATE) / sampleRate;

    for (u16 outputIdx = 0; outputIdx < FFT_SAMPLE_COUNT; outputIdx++) {
        const f64 mappedInputIdx = outputIdx / resampleRatio;
        const i32 centerIdx = i32(std::floor(mappedInputIdx));

        f64 sum = 0.0;
        f64 norm = 0.0;

        for (i32 kernelOffset = -FILTER_RADIUS; kernelOffset <= FILTER_RADIUS;
             kernelOffset++) {
            const i32 idx = centerIdx + kernelOffset;

            if (idx < 0 || idx >= sampleCount) {
                continue;
            }

            const f64 distance = mappedInputIdx - idx;

            const f64 window =
                blackman(kernelOffset + FILTER_RADIUS, FILTER_SIZE);
            const f64 filterTap = cutoff * sinc(distance * cutoff) * window;

            sum += input[idx] * filterTap;
            norm += filterTap;
        }

        output[outputIdx] = (norm != 0.0) ? f32(sum / norm) : 0.0F;
    }
}