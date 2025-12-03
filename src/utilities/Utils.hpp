#pragma once

#include "Aliases.hpp"
#include "Enums.hpp"
#include "FWD.hpp"

struct EqualizerPreset {
    QString name;
    Bands bandCount;

    array<i8, THIRTY_BANDS> bands;
};

struct CUETrack {
    QString title;
    QString artist;
    QString trackNumber;
    u16 offset;
};

struct CUEInfo {
    TrackMetadata metadata;
    vector<CUETrack> tracks;
    QString cueFilePath;
};

static thread_local u32 seed =
    duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    )
        .count();

auto timeToSecs(const QString& time) -> u16;
auto secsToMins(u16 totalSeconds) -> QString;

auto trackPropertiesLabels() -> QStringList;

auto randint() -> u32;
auto randint_range(u32 min, u32 max) -> u32;

auto parsePreset(const QByteArray& presetData)
    -> result<EqualizerPreset, QString>;
auto serializePreset(const EqualizerPreset& preset) -> QByteArray;

constexpr auto getXSPFTag(TrackProperty property) -> QStringView;
auto exportXSPF(
    const QString& outputPath,
    const vector<TrackMetadata>& metadataVector
) -> result<bool, QString>;
auto exportM3U8(
    const QString& outputPath,
    const vector<TrackMetadata>& metadataVector
) -> result<bool, QString>;
auto parseCUE(QFile& cueFile, const QFileInfo& fileInfo) -> CUEInfo;

auto applyEffectToImage(
    const QImage& src,
    QGraphicsEffect* effect,
    i32 extent = 0
) -> QImage;

auto FFmpegError(cstr file, i32 line, cstr func, i32 err) -> QString;
#define FFMPEG_ERROR(err) FFmpegError(__FILE__, __LINE__, __func__, err)

// TODO: Extract sample format
auto extractMetadata(const QString& filePath) -> result<TrackMetadata, QString>;
auto extractCover(cstr path) -> result<vector<u8>, QString>;
