#pragma once

#include "aliases.hpp"
#include "constants.hpp"
#include "ffmpeg.hpp"

#include <juce_dsp/juce_dsp.h>

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

using IIRFilter = juce::dsp::IIR::Filter<f32>;
using IIRCoefficients = juce::dsp::IIR::Coefficients<f32>;

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    explicit AudioStreamer(QObject* parent = nullptr);

    [[nodiscard]] constexpr auto duration() const -> u16 {
        return secondsDuration;
    };

    [[nodiscard]] constexpr auto format() const -> QAudioFormat {
        return format_;
    };

    [[nodiscard]] constexpr auto bytesAvailable() const -> qi64 override {
        return nextBufferSize;
    };

    [[nodiscard]] constexpr auto atEnd() const -> bool override {
        return nextBufferSize == 0;
    };

    void seekSecond(u16 second);
    void start(const QString& path);
    auto reset() -> bool override;

    constexpr void setGain(const i8 dbGain, const u8 band) {
        gains_[band] = dbGain;
        changedBands[band] = true;
    };

    constexpr void setFrequency(const f32 frequency, const u8 band) {
        frequencies[band] = frequency;
        changedBands[band] = true;
    }

    [[nodiscard]] constexpr auto gain(const u8 band) const -> i8 {
        return gains_[band];
    };

    [[nodiscard]] constexpr auto gains() const
        -> const array<i8, THIRTY_BANDS>& {
        return gains_;
    };

    void setBandCount(u8 bands);

    [[nodiscard]] constexpr auto bands() const
        -> const array<f32, THIRTY_BANDS>& {
        return frequencies;
    };

    [[nodiscard]] auto equalizerEnabled() const -> bool { return eqEnabled; };

    constexpr void toggleEqualizer(const bool enabled) { eqEnabled = enabled; };

   signals:
    void progressUpdate(u16 second);
    void streamEnded();

   protected:
    auto readData(str data, qi64 /* size */) -> qi64 override;

    auto writeData(cstr /* data */, qi64 /* size */) -> qi64 override {
        return -1;
    };

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
    QAudioFormat format_;

    i64 nextBufferSize = 0;
    i32 audioStreamIndex = 0;
    u16 secondsDuration = 0;
    u16 playbackSecond = 0;

    bool eqEnabled = false;

    u8 bandCount = TEN_BANDS;

    array<f32, THIRTY_BANDS> frequencies;
    array<i8, THIRTY_BANDS> gains_;
    array<bool, THIRTY_BANDS> changedBands;
    vector<vector<IIRFilter>> filters;
};
