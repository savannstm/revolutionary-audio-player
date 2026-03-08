#pragma once

#include "Aliases.hpp"
#include "FFMpeg.hpp"
#include "FWD.hpp"
#include "Settings.hpp"

using namespace FFmpeg;

class AudioStreamer {
   public:
    explicit AudioStreamer(const shared_ptr<Settings>& settings);

    ~AudioStreamer() { delete[] buf; }

    struct Block {
        span<const u8> firstPart;
        span<const u8> secondPart;
        u32 second;
    };

    auto start(const string& path, i32 startSecond)
        -> result<std::monostate, QString>;
    auto reset() -> bool;

    void seekSecond(i32 second);

    [[nodiscard]] constexpr auto sampleRate() const -> u32 {
        return codecContext->sample_rate;
    }

    [[nodiscard]] constexpr auto channels() const -> u8 {
        return codecContext->ch_layout.nb_channels;
    }

    constexpr void changeGain() { changedGains = true; }

    [[nodiscard]] constexpr auto copySize() const -> u16 { return copySize_; }

    [[nodiscard]] auto consumeBlock() -> optional<Block>;

    constexpr void flush() {
        headOffset = 0;
        tailOffset = 0;
        dataSize.store(0, std::memory_order_release);
        samplePos.store(0, std::memory_order_release);
    }

    void readData();
    void addFFTSamples(u32 offset, u32 size);

   private:
    inline void readRaw();
    inline void equalizeData(u32 offset, u32 size);
    inline void readPlanar1Ch(
        f32* __restrict dst,
        const u8* __restrict const* __restrict src,
        u32 firstChunk,
        u32 secondChunk
    );
    inline void readPlanarMultichannel(
        f32* __restrict dst,
        const u8* __restrict const* __restrict src,
        u32 firstChunk,
        u32 secondChunk
    );
    inline void readFrame();
    inline void initializeFilters(bool force = false);
    [[nodiscard]] inline auto buildEqualizerArgs() const -> string;
    [[nodiscard]] inline auto
    checkError(i32 err, bool resetStreamer, bool resetFilters) -> bool;
    inline void writeToRingBuf(u32 offset, const void* src, u32 size);
    inline void readFromRingBuf(u32 offset, void* dst, u32 size);

    inline void buildPeaks();

    static constexpr u32 RAW_BUFFER_THRESHOLD = (UINT16_MAX + 1) * 2;
    static constexpr u16 MIN_BUFFER_THRESHOLD = 8192;
    static constexpr u16 MIN_BUF_SIZE = 16384;
    static constexpr u16 MAX_OPUS_FRAME_SAMPLES = 5760;
    static constexpr u16 MAX_VORBIS_FRAME_SAMPLES = 8192;
    static constexpr u16 MAX_AC3_FRAME_SAMPLES = 1536;
    static constexpr cstr F32_SAMPLE_FORMAT_NAME = "flt";

    EqualizerSettings& eqSettings;
    SpectrumVisualizerSettings& visSettings;

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

    u64 rawEndPos;

    u8* __restrict buf = nullptr;
    u32 bufferThreshold;
    u32 bufSize;
    u32 bufSizeMask;

    u32 headOffset;
    u32 tailOffset;
    atomicU32 dataSize;
    atomicU32 samplePos;

    u16 copySize_;

    SampleSize sampleSize;

    u8 audioStreamIndex;

    bool rawPCM = false;

    bool changedGains = false;
    bool streamFinished_ = false;
    bool seekPerformed = false;
};
