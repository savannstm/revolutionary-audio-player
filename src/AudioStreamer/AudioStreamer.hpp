#pragma once

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
}

#include "Aliases.hpp"
#include "Constants.hpp"
#include "ExtractMetadata.hpp"
#include "FFMpeg.hpp"
#include "Logger.hpp"

#include <QAudioFormat>
#include <QIODevice>

using namespace FFmpeg;

// TODO: Playback stutters seem to only happen when I/O can't keep up
// A solution for that is deque that asynchronously reads buffers, but that
// requires thinking

class AudioStreamer : public QIODevice {
    Q_OBJECT

   public:
    explicit AudioStreamer(QObject* parent = nullptr);

    [[nodiscard]] constexpr auto format() const -> QAudioFormat {
        return format_;
    };

    [[nodiscard]] constexpr auto bytesAvailable() const -> qi64 override {
        // We don't care about how many bytes are there available since we're
        // just reading the file until the end and handling EOF at the spot
        return UINT16_MAX;
    };

    [[nodiscard]] constexpr auto atEnd() const -> bool override {
        // Same from `bytesAvailable` applies
        return false;
    };

    void seekSecond(u16 second);
    void start(const QString& path);
    auto reset() -> bool override;

    constexpr void setGain(const i8 dbGain, const u8 band) {
        gains_[band] = dbGain;
        changedBands = true;

        initializeFilters();
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

        if (equalizerEnabled_) {
            initializeFilters();
        }
    };

    [[nodiscard]] constexpr auto progress() const -> u16 {
        return playbackSecond;
    }

    vector<u8>* visualizerBuffer = nullptr;

   signals:
    void progressUpdate(u16 second);
    void streamEnded();
    void buildPeaks(u16 sampleRate);

   protected:
    auto readData(str data, qi64 maxSize) -> qi64 override;

    auto writeData(cstr /* data */, qi64 /* size */) -> qi64 override {
        return -1;
    };

   private:
    inline auto prepareBuffer(str buf) -> i64;
    inline auto decodeRaw(str buf) -> i64;
    inline auto processFrame(i64 totalRead, str buf) -> i64;
    [[nodiscard]] inline auto second() const -> u16;
    inline void equalizeFrame();
    void initializeFilters();
    inline void convertFrame();
    [[nodiscard]] inline auto buildEqualizerArgs() const -> string;
    inline void convertBuffer(vector<u8>& buffer, u32 bytesRead);
    inline void equalizeBuffer(vector<u8>& buffer);

    auto
    checkError(const i32 err, const bool resetStreamer, const bool resetFilters)
        -> bool {
        if (err < 0) {
            LOG_WARN(FFMPEG_ERROR(err));

            if (resetStreamer) {
                reset();
            }

            if (resetFilters) {
                filterGraph.reset();
            }

            return true;
        }

        return false;
    };

    FormatContext formatContext;
    CodecContext codecContext;
    FFmpeg::SwrContext swrContext;
    Packet packet;
    Frame frame;

    FilterGraph filterGraph;
    AVFilterContext* abufferContext = nullptr;
    AVFilterContext* equalizerContext = nullptr;
    AVFilterContext* limiterContext = nullptr;
    AVFilterContext* aformatContext = nullptr;
    AVFilterContext* abuffersinkContext = nullptr;

    QAudioFormat format_;

    u8 audioStreamIndex = 0;
    u16 playbackSecond = 0;
    u16 lastPlaybackSecond = 0;

    bool rawPCM = false;
    u8 inputSampleSize = 0;
    u32 fileKbps = 0;

    u8 bandCount = TEN_BANDS;

    FrequencyArray frequencies_;
    GainArray gains_;

    bool changedBands = false;
    bool equalizerEnabled_ = false;
};
