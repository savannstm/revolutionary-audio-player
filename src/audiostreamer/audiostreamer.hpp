#pragma once

#include "aliases.hpp"
#include "ffmpeg.hpp"

#include <juce_dsp/juce_dsp.h>

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

constexpr u8 EQ_BANDS_N = 10;
constexpr array<f32, EQ_BANDS_N> FREQUENCES = { 31.0,   62.0,   125.0,  250.0,
                                                500.0,  1000.0, 2000.0, 4000.0,
                                                8000.0, 16000.0 };
constexpr f32 QFactor = 1.0;

using gains_array = array<u8, EQ_BANDS_N>;

// TODO: Implement non-blocking `readData`

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    AudioStreamer(QObject* parent = nullptr) : QIODevice(parent) {
        audioFormat.setSampleFormat(QAudioFormat::Float);
    }

    ~AudioStreamer() override = default;

    [[nodiscard]] auto duration() const -> u16;
    [[nodiscard]] auto format() const -> QAudioFormat;
    [[nodiscard]] auto isSequential() const -> bool override;
    [[nodiscard]] auto bytesAvailable() const -> qi64 override;
    [[nodiscard]] auto second() const -> u16;
    [[nodiscard]] auto atEnd() const -> bool override;
    auto seekSecond(u16 second) -> bool;
    auto start(const path& path) -> bool;
    auto reset() -> bool override;

    void updateEq(bool enable, const gains_array& gains);

   signals:
    void progressUpdate(u16 second);
    void endOfFile();

   protected:
    auto readData(char* data, qi64 maxSize) -> qi64 override;
    auto writeData(const char* /* data */, qi64 /* size */) -> qi64 override;

   private:
    inline void equalizeBuffer(QByteArray& buffer);
    inline void prepareBuffer();

    FormatContextPtr formatContext;
    CodecContextPtr codecContext;
    SwrContextPtr swrContext;
    PacketPtr packet;
    FramePtr frame;

    QByteArray buffer;
    QAudioFormat audioFormat;

    i64 nextBufferSize = 0;
    i32 audioStreamIndex = 0;
    u16 secondsDuration = 0;
    u16 playbackSecond = 0;

    bool eqEnabled;
    gains_array eqGains;
    vector<array<juce::dsp::IIR::Filter<f32>, EQ_BANDS_N>> filters;
};
