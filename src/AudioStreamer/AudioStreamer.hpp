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
#include "Settings.hpp"

#include <miniaudio.h>

using namespace FFmpeg;

// TODO: Playback stutters seem to only happen when I/O can't keep up
// When tested with tracks located on HDD, stutters, on SSD it doesn't
// A solution for that is deque that asynchronously reads buffers, but that
// requires thinking

class AudioStreamer : public QObject {
    Q_OBJECT

   public:
    explicit AudioStreamer(
        shared_ptr<Settings> settings,
        QObject* parent = nullptr
    );

    void seekSecond(u16 second);
    void start(const QString& path, u16 startSecond);
    auto reset() -> bool;

    [[nodiscard]] constexpr auto progress() const -> u16 {
        return playbackSecond;
    }

    [[nodiscard]] constexpr auto sampleRate() const -> u32 {
        return sampleRate_;
    }

    [[nodiscard]] constexpr auto channels() const -> AudioChannels {
        return channels_;
    }

    auto readData(u8* output) -> u64;

    void changeGain(const u8 band) { changedGains = true; }

    void togglePeakVisualizer(const bool enabled) {
        peakVisualizerEnabled = enabled;
    }

    void toggleVisualizer(const bool enabled) { visualizerEnabled = enabled; }

    f32* peakVisualizerBuffer;
    f32* visualizerBuffer;

   signals:
    void streamEnded();
    void processedSamples();
    void progressUpdate(u16 second);

   private:
    inline void prepareBuffer();
    inline void decodeRaw();
    inline void convertBuffer(u32 bytesRead);
    inline void equalizeBuffer();
    inline void processFrame();
    inline void initializeFilters();
    [[nodiscard]] inline auto buildEqualizerArgs() const -> string;

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

    array<u8, UINT16_MAX + 1> arrayBuffer;
    array<u8, MIN_BUFFER_SIZE> leftoverBuffer;
    vector<u8> buffer;

    usize bufferSize = 0;
    usize bufferOffset = 0;

    // FLAC/ALAC format may contain frames as big as 2 MB - but all lossy
    // formats have frames less than UINT16_MAX even for highest available
    // channel count and sample rate. So we keep array buffer as our main
    // buffer, and make vector buffer our FLAC/ALAC buffer.
    u8* buf = nullptr;

    FormatContext formatContext;
    CodecContext codecContext;
    Packet packet;
    Frame frame;

    FilterGraph filterGraph;
    AVFilterContext* abufferContext = nullptr;
    AVFilterContext* equalizerContext = nullptr;
    AVFilterContext* limiterContext = nullptr;
    AVFilterContext* aformatContext = nullptr;
    AVFilterContext* abuffersinkContext = nullptr;

    shared_ptr<Settings> settings;
    EqualizerSettings& eqSettings;

    u32 sampleRate_ = 0;
    u32 fileKbps = 0;

    u16 leftoverSize = 0;
    u16 minBufferSize = 0;

    u16 playbackSecond = 0;
    u16 lastPlaybackSecond = 0;

    AudioChannels channels_;

    u8 audioStreamIndex = 0;
    bool planarFormat = false;
    bool rawPCM = false;
    bool encodedLoselessFormat = false;
    SampleSize sampleSize;

    bool peakVisualizerEnabled = false;
    bool visualizerEnabled = false;
    bool changedGains = false;
};
