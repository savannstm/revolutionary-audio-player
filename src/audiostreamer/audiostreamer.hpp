#pragma once

#include "aliases.hpp"
#include "ffmpeg.hpp"

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    AudioStreamer(QObject* parent = nullptr) : QIODevice(parent) {
        audioFormat.setSampleFormat(QAudioFormat::Int16);
    }

    ~AudioStreamer() override = default;

    [[nodiscard]] auto duration() const -> u16;
    [[nodiscard]] auto format() const -> QAudioFormat;
    [[nodiscard]] auto pos() const -> i64 override;
    [[nodiscard]] auto isSequential() const -> bool override;
    [[nodiscard]] auto bytesAvailable() const -> i64 override;
    [[nodiscard]] auto second() const -> u16;
    auto seekSecond(u16 second) -> bool;
    auto start(const path& path) -> bool;
    auto reset() -> bool override;

   protected:
    auto readData(char* data, i64 maxSize) -> i64 override;
    auto writeData(const char* /* data */, i64 /* size */) -> i64 override;

   private:
    FormatContextPtr formatContext;
    CodecContextPtr codecContext;
    SwrContextPtr swrContext;
    PacketPtr packet;
    FramePtr frame;

    QByteArray buffer;
    QAudioFormat audioFormat;

    i32 audioStreamIndex = 0;
    i64 bufferPosition = 0;
    i64 bytesPerSecond = 0;
    u16 secondsDuration = 0;
    u16 playbackSecond = 0;
    i64 byteOffset = 0;
};