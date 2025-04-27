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

using IIRFilter = juce::dsp::IIR::Filter<f32>;
using IIRCoefficients = juce::dsp::IIR::Coefficients<f32>;

constexpr frequencies_array TEN_BAND_FREQUENCIES = { 31,   62,   125,  250,
                                                     500,  1000, 2000, 4000,
                                                     8000, 16000 };

constexpr frequencies_array THREE_BAND_FREQUENCIES = { 100, 1000, 10000 };

constexpr frequencies_array FIVE_BAND_FREQUENCIES = { 60,
                                                      250,
                                                      1000,
                                                      4000,
                                                      16000 };

constexpr u8 THIRTY_BANDS = 30;
constexpr u8 EIGHTEEN_BANDS = 18;
constexpr u8 TEN_BANDS = 10;
constexpr u8 FIVE_BANDS = 5;
constexpr u8 THREE_BANDS = 3;

// TODO: Implement 18 and 30 bands equalizers;

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

    [[nodiscard]] constexpr auto gain(const u8 band) const -> i8 {
        return gains_[band];
    };

    [[nodiscard]] constexpr auto gains() const -> const vector<i8>& {
        return gains_;
    };

    void setBands(u8 count);

    [[nodiscard]] constexpr auto bands() const -> const vector<f32>& {
        return frequencies;
    };

    [[nodiscard]] auto isEqEnabled() const -> bool { return eqEnabled; };

    constexpr void toggleEqualizer(const bool enabled) { eqEnabled = enabled; };

   signals:
    void progressUpdate(u16 second);
    void endOfFile();

   protected:
    auto readData(str data, qi64 maxSize) -> qi64 override;

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

    u8 bandCount = EQ_BANDS_N;
    vector<f32> frequencies =
        vector(TEN_BAND_FREQUENCIES.begin(), TEN_BAND_FREQUENCIES.end());

    vector<bool> changedBands = { false, false, false, false, false,
                                  false, false, false, false, false };

    vector<i8> gains_ = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    array<vector<IIRFilter>, CHANNEL_N> filters;
};
