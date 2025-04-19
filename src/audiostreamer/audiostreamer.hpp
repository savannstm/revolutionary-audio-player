#pragma once

#include "aliases.hpp"
#include "ffmpeg.hpp"

#include <juce_dsp/juce_dsp.h>

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

constexpr u8 EQ_BANDS_N = 10;
constexpr array<f32, EQ_BANDS_N> FREQUENCIES = { 31.0,   62.0,   125.0,  250.0,
                                                 500.0,  1000.0, 2000.0, 4000.0,
                                                 8000.0, 16000.0 };
constexpr f32 QFactor = 1.0;
constexpr u8 CHANNEL_N = 2;

using db_gains_array = array<i8, EQ_BANDS_N>;

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    explicit AudioStreamer(QObject* parent = nullptr);
    ~AudioStreamer() override = default;

    [[nodiscard]] auto duration() const -> u16;
    [[nodiscard]] auto getFormat() const -> QAudioFormat;
    [[nodiscard]] auto bytesAvailable() const -> qi64 override;
    [[nodiscard]] auto atEnd() const -> bool override;
    auto seekSecond(u16 second) -> bool;
    auto start(const path& path) -> bool;
    auto reset() -> bool override;

    void setGain(i8 dbGain, u8 band);
    auto getGain(u8 band) -> i8;

    [[nodiscard]] auto isEqEnabled() const -> bool;
    void setEqEnabled(bool enabled);

   signals:
    void progressUpdate(u16 second);
    void endOfFile();

   protected:
    auto readData(char* data, qi64 maxSize) -> qi64 override;
    auto writeData(const char* /* data */, qi64 /* size */) -> qi64 override;

   private:
    inline void equalizeBuffer(QByteArray& buffer);
    inline void prepareBuffer();
    inline void initFilters();
    [[nodiscard]] inline auto second() const -> u16;

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

    bool eqEnabled = false;

    array<bool, EQ_BANDS_N> changedBands = {
        false, false, false, false, false, false, false, false, false, false
    };

    db_gains_array dbGains = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    //! ZAMN 800 BYTES
    array<array<juce::dsp::IIR::Filter<f32>, EQ_BANDS_N>, CHANNEL_N> filters;
};
