#pragma once

#include "aliases.hpp"
#include "ffmpeg.hpp"

#include <juce_dsp/juce_dsp.h>

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

constexpr u8 EQ_BANDS_N = 10;
constexpr f32 QFactor = 1.0;
constexpr u8 CHANNEL_N = 2;

using db_gains_array = array<i8, EQ_BANDS_N>;
using frequencies_array = array<f32, EQ_BANDS_N>;

constexpr frequencies_array TEN_BAND_FREQUENCIES = { 31,   62,   125,  250,
                                                     500,  1000, 2000, 4000,
                                                     8000, 16000 };

constexpr frequencies_array THREE_BAND_FREQUENCIES = { 100, 1000, 10000, 0, 0,
                                                       0,   0,    0,     0, 0 };

constexpr frequencies_array FIVE_BAND_FREQUENCIES = { 60,    250, 1000, 4000,
                                                      16000, 0,   0,    0,
                                                      0,     0 };

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    explicit AudioStreamer(QObject* parent = nullptr);

    [[nodiscard]] auto duration() const -> u16;
    [[nodiscard]] auto getFormat() const -> QAudioFormat;
    [[nodiscard]] auto bytesAvailable() const -> qi64 override;
    [[nodiscard]] auto atEnd() const -> bool override;
    auto seekSecond(u16 second) -> bool;
    auto start(const QString& path) -> bool;
    auto reset() -> bool override;

    void setGain(i8 dbGain, u8 band);
    auto getGain(u8 band) -> i8;
    auto gains() -> const db_gains_array&;

    void setBands(u8 count);
    auto bands() -> const frequencies_array&;

    [[nodiscard]] auto isEqEnabled() const -> bool;
    void setEqEnabled(bool enabled);

   signals:
    void progressUpdate(u16 second);
    void endOfFile();

   protected:
    auto readData(str data, qi64 maxSize) -> qi64 override;
    auto writeData(cstr /* data */, qi64 /* size */) -> qi64 override;

   private:
    inline void equalizeBuffer(QByteArray& buf);
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

    u8 bandCount = EQ_BANDS_N;
    frequencies_array frequencies = TEN_BAND_FREQUENCIES;

    array<bool, EQ_BANDS_N> changedBands = {
        false, false, false, false, false, false, false, false, false, false
    };

    db_gains_array dbGains = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    array<array<unique_ptr<juce::dsp::IIR::Filter<f32>>, EQ_BANDS_N>, CHANNEL_N>
        filters;
};
