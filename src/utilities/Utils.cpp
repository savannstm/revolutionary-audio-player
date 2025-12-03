#include "Utils.hpp"

#include "Constants.hpp"
#include "FFMpeg.hpp"

#include <QDir>
#include <QFileInfo>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QObject>
#include <QPainter>
#include <QTextStream>

using namespace FFmpeg;

auto timeToSecs(const QString& time) -> u16 {
    const QStringList parts = time.split(':');
    u16 seconds;

    if (parts.size() == 3) {
        const u16 hours = parts[0].toUInt();
        const u16 minutes = parts[1].toUInt();
        const u16 secs = parts[2].toUInt();
        seconds = (hours * HOUR_SECONDS) + (minutes * MINUTE_SECONDS) + secs;
    } else if (parts.size() == 2) {
        const u16 minutes = parts[0].toUInt();
        const u16 secs = parts[1].toUInt();
        seconds = (minutes * MINUTE_SECONDS) + secs;
    }

    return seconds;
}

auto secsToMins(const u16 totalSeconds) -> QString {
    const u16 minutes = (totalSeconds % HOUR_SECONDS) / MINUTE_SECONDS;
    const u16 seconds = totalSeconds % MINUTE_SECONDS;

    QString result;
    result.reserve(5);

    result.append(QChar('0' + (minutes / 10)));
    result.append(QChar('0' + (minutes % 10)));

    result.append(':');

    result.append(QChar('0' + (seconds / 10)));
    result.append(QChar('0' + (seconds % 10)));

    return result;
}

auto trackPropertiesLabels() -> QStringList {
    return { QString(),
             QObject::tr("Title"),
             QObject::tr("Artist"),
             QObject::tr("Track Number"),
             QObject::tr("Album"),
             QObject::tr("Album Artist"),
             QObject::tr("Genre"),
             QObject::tr("Year"),
             QObject::tr("Duration"),
             QObject::tr("Composer"),
             QObject::tr("BPM"),
             QObject::tr("Language"),
             QObject::tr("Disc Number"),
             QObject::tr("Comment"),
             QObject::tr("Publisher"),
             QObject::tr("Bitrate"),
             QObject::tr("Sample Rate"),
             QObject::tr("Channels"),
             QObject::tr("Format"),
             QObject::tr("Path"),
             QObject::tr("Order") };
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

    for (const u8 band : range(0, u8(preset.bandCount))) {
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

constexpr auto getXSPFTag(const TrackProperty property) -> QStringView {
    switch (property) {
        case TrackProperty::Title:
            return u"title"_qsv;
        case TrackProperty::Artist:
            return u"creator"_qsv;
        case TrackProperty::Album:
            return u"album"_qsv;
        case TrackProperty::AlbumArtist:
            return u"albumArtist"_qsv;
        case TrackProperty::Genre:
            return u"genre"_qsv;
        case TrackProperty::Duration:
            return u"duration"_qsv;
        case TrackProperty::TrackNumber:
            return u"trackNum"_qsv;
        case TrackProperty::Comment:
            return u"annotation"_qsv;
        case TrackProperty::Path:
            return u"location"_qsv;
        case TrackProperty::Composer:
            return u"composer"_qsv;
        case TrackProperty::Publisher:
            return u"publisher"_qsv;
        case TrackProperty::Year:
            return u"year"_qsv;
        case TrackProperty::BPM:
            return u"bpm"_qsv;
        case TrackProperty::Language:
            return u"language"_qsv;
        case TrackProperty::DiscNumber:
            return u"disc"_qsv;
        case TrackProperty::Bitrate:
            return u"bitrate"_qsv;
        case TrackProperty::SampleRate:
            return u"samplerate"_qsv;
        case TrackProperty::Channels:
            return u"channels"_qsv;
        case TrackProperty::Format:
            return u"format"_qsv;
        default:
            return {};
            break;
    }
}

auto exportXSPF(
    const QString& outputPath,
    const vector<TrackMetadata>& metadataVector
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

    for (const auto& metadata : metadataVector) {
        output << "<track>";
        output << '\n';

        for (const auto& [property, value] : views::drop(metadata, 1)) {
            const QStringView tag = getXSPFTag(property);

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
        const QString durationStr = metadata[TrackProperty::Duration];

        u16 duration = 0;

        const QStringList strings = durationStr.split(':');
        const QString& minutes = strings[0];
        const QString& seconds = strings[1];

        duration += minutes.toUInt() * MINUTE_SECONDS;
        duration += seconds.toUInt();

        output << "#EXTINF:";
        output << duration << ',' << artist << " - " << title;
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
    u16 offset = 0;

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

    QPainter painter = QPainter(&res);
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

    const i8 audioStreamIndex = i8(err);
    const AVStream* const audioStream =
        formatContext->streams[audioStreamIndex];

    const auto getTag = [&formatContext](cstr key) -> QString {
        const AVDictionaryEntry* const tag =
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

    const QString bitrateString = QString::number(bitrate) + u"k"_qssv;

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
