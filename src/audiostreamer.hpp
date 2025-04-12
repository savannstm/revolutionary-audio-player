#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include "aliases.hpp"
#include "ffmpeg.hpp"

#include <QAudioFormat>
#include <QIODevice>
#include <QLabel>

using namespace FFmpeg;

class AudioStreamer : public QIODevice {
   public:
    AudioStreamer(QObject* parent = nullptr) : QIODevice(parent) {
        audioFormat.setSampleFormat(QAudioFormat::Int16);
    }

    ~AudioStreamer() override = default;

    [[nodiscard]] auto duration() const -> u16;
    [[nodiscard]] auto format() const -> QAudioFormat;
    [[nodiscard]] auto pos() const -> i64 override;
    [[nodiscard]] auto size() const -> i64 override;
    [[nodiscard]] auto bytesRead() const -> i64;
    auto seek(i64 sec) -> bool override;
    auto start(const path& path) -> bool;
    auto reset() -> bool override;

   protected:
    auto readData(char* data, i64 maxSize) -> i64 override;
    auto writeData(const char* /* data */, i64 /* size */) -> i64 override;
    [[nodiscard]] auto bytesAvailable() const -> i64 override;

   private:
    FormatContextPtr formatContext;
    CodecContextPtr codecContext;
    AVChannelLayout channelLayout;
    SwrContextPtr swrContext;
    PacketPtr packet;
    FramePtr frame;
    QByteArray buffer;
    QAudioFormat audioFormat;
    i32 audioStreamIndex = 0;
    i64 position = 0;
    u16 secondsDuration = 0;
    i64 availableBytes = 0;
    u8 channels = 0;
    u16 sampleRate = 0;
    u32 frames = 0;
    i64 numBytesRead = 0;
};