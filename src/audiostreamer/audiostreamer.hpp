#pragma once

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
}

#include "aliases.hpp"
#include "constants.hpp"
#include "ffmpeg.hpp"

#include <QAudioFormat>
#include <QDebug>
#include <QIODevice>

using namespace FFmpeg;

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

    [[nodiscard]] constexpr auto gains() const -> const GainArray& {
        return gains_;
    };

    void setBandCount(u8 bands);

    [[nodiscard]] constexpr auto frequencies() const -> const FrequencyArray& {
        return frequencies_;
    };

    [[nodiscard]] auto equalizerEnabled() const -> bool {
        return equalizerEnabled_;
    };

    constexpr void toggleEqualizer(const bool enabled) {
        equalizerEnabled_ = enabled;
    };

    [[nodiscard]] constexpr auto progress() const -> u16 {
        return playbackSecond;
    }

    void printErr(const i32 err) {
        qWarning() << av_make_error_string(errBuf.data(), errBuf.size(), err);
    }

   signals:
    void progressUpdate(u16 second);
    void streamEnded();

   protected:
    auto readData(str data, qi64 maxSize) -> qi64 override;

    auto writeData(cstr /* data */, qi64 /* size */) -> qi64 override {
        return -1;
    };

   private:
    inline void prepareBuffer();
    inline void decodeRaw();
    inline auto processFrame() -> bool;
    [[nodiscard]] inline auto second() const -> u16;
    inline void equalizeFrame();
    inline void initializeFilters();
    inline void convertFrame();
    [[nodiscard]] inline auto buildEqualizerArgs(bool change) -> string;

    FormatContext formatContext;
    CodecContext codecContext;
    FFmpeg::SwrContext swrContext;
    Packet packet;
    Frame frame;

    FilterGraph filterGraph;
    AVFilterContext* abufferContext = nullptr;
    AVFilterContext* equalizerContext = nullptr;
    AVFilterContext* normalizerContext = nullptr;
    AVFilterContext* aformatContext = nullptr;
    AVFilterContext* abuffersinkContext = nullptr;

    QByteArray buffer;
    QAudioFormat format_;

    u32 nextBufferSize = 0;
    i8 audioStreamIndex = 0;
    u16 secondsDuration = 0;
    u16 playbackSecond = 0;

    // For raw formats
    u32 totalBytesRead = 0;
    u8 inputSampleSize = 0;
    u32 bytesPerSecond = 0;

    string_view formatName;

    u8 bandCount = TEN_BANDS;

    FrequencyArray frequencies_;
    GainArray gains_;

    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    array<bool, MAX_BANDS_COUNT> changedBands;

    bool equalizerEnabled_ = false;
    bool bandCountChanged = false;
};
