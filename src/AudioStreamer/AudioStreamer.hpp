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

struct Buffer {
    vector<u8> buf;
    u16 second;
};

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

    [[nodiscard]] constexpr auto sampleRate() const -> u32 {
        return sampleRate_;
    }

    [[nodiscard]] constexpr auto channels() const -> AudioChannels {
        return channels_;
    }

    void changeGain(const u8 band) { changedGains = true; }

    void prepareBuffer();

    void flushBuffers() { preparedBuffers = 0; };

    array<Buffer, BUFFERS_QUEUE_SIZE> buffers{};

    usize bufferSize = 0;
    usize preparedBuffers = 0;

    u16 minBufferSize = 0;
    alignas(4) bool encodedLoselessFormat = false;

   signals:
    void streamEnded();
    void processedSamples();
    void progressUpdate(u16 second);

   private:
    inline void decodeRaw();
    inline void convertBuffer();
    inline void equalizeBuffer();
    inline void processFrame();
    inline void initializeFilters(bool force = false);
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

    array<u8, MIN_BUFFER_SIZE> leftoverBuffer;
    Buffer* buffer;

    usize bufferOffset = 0;

    const AVStream* audioStream;

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
    u16 rawBufferSize = 0;

    AudioChannels channels_;
    SampleSize sampleSize;

    u8 audioStreamIndex = 0;

    bool planarFormat = false;
    bool rawPCM = false;

    bool changedGains = false;
};
