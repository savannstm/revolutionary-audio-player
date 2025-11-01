#include "AudioStreamer.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"
#include "FFMpeg.hpp"

struct Filter {
    cstr filter;
    cstr name;
    cstr args;
    AVFilterContext** context;
};

AudioStreamer::AudioStreamer(shared_ptr<Settings> settings_, QObject* parent) :
    QObject(parent),
    settings(std::move(settings_)),
    eqSettings(settings->equalizerSettings) {
#ifdef DEBUG_BUILD
    av_log_set_level(AV_LOG_VERBOSE);
#elifdef RELEASE_BUILD
    av_log_set_level(AV_LOG_QUIET);
#endif
}

void AudioStreamer::start(const QString& path, const u16 startSecond) {
    reset();

    AVFormatContext* fCtxPtr = nullptr;

    i32 err = avformat_open_input(
        &fCtxPtr,
        path.toStdString().c_str(),
        nullptr,
        nullptr
    );

    if (checkError(err, true, false)) {
        return;
    }

    formatContext.reset(fCtxPtr);
    const i64 headerLength = avio_tell(formatContext->pb);

    err = avformat_find_stream_info(fCtxPtr, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    const AVCodec* codec = nullptr;
    err = av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

    if (checkError(err, true, false)) {
        return;
    }

    audioStreamIndex = u8(err);

    const AVStream* audioStream = fCtxPtr->streams[audioStreamIndex];

    codecContext = createCodecContext(codec);
    avcodec_parameters_to_context(codecContext.get(), audioStream->codecpar);
    err = avcodec_open2(codecContext.get(), codec, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    const AVChannelLayout& channelLayout = codecContext->ch_layout;
    const auto channels = AudioChannels(codecContext->ch_layout.nb_channels);
    const u32 sampleRate = codecContext->sample_rate;

    sampleRate_ = sampleRate;
    channels_ = channels;

    const string_view formatName = codecContext->codec->name;
    rawPCM = formatName.starts_with("pcm");

    sampleSize = SampleSize(av_get_bytes_per_sample(codecContext->sample_fmt));

    if (rawPCM) {
        // Ensure that we are at the very start of the data,
        // when reading raw PCM
        avio_seek(formatContext->pb, headerLength, SEEK_SET);

        if (formatName.ends_with("24le")) {
            sampleSize = SampleSize::S24;
        }

        planarFormat = false;
        encodedLoselessFormat = false;
        buf = arrayBuffer.data();
        fileKbps = sampleRate * channels * sampleSize;
    } else {
        encodedLoselessFormat = formatName == "flac" || formatName == "alac";
        planarFormat = bool(av_sample_fmt_is_planar(codecContext->sample_fmt));

        if (encodedLoselessFormat) {
            buf = buffer.data();
        } else {
            buf = arrayBuffer.data();
        }

        // We need that only in encoded formats
        packet = createPacket();
        frame = createFrame();
    }

    minBufferSize = (channels_ == AudioChannels::Surround51)
                        ? MIN_BUFFER_SIZE_3BYTES
                        : MIN_BUFFER_SIZE;

    if (startSecond != UINT16_MAX) {
        seekSecond(startSecond);
    }

    initializeFilters();
}

//! maxSize should exactly match minBufferSize, so we don't use it
auto AudioStreamer::readData(u8* output) -> u64 {
    if (bufferSize == 0 || bufferOffset >= bufferSize) {
        prepareBuffer();
        bufferOffset = 0;

        if (bufferSize == 0) {
            emit streamEnded();
            return 0;
        }
    }

    const u64 bytesToCopy =
        std::min<u64>(minBufferSize, bufferSize - bufferOffset);
    u8* src;

    if (encodedLoselessFormat) {
        src = buffer.data() + bufferOffset;
    } else {
        src = arrayBuffer.data() + bufferOffset;
    }

    // First we copy full-volume samples into the visualizers
    if (peakVisualizerEnabled) {
        memcpy(peakVisualizerBuffer, src, bytesToCopy);
    }

    if (visualizerEnabled) {
        memcpy(visualizerBuffer, src, bytesToCopy);
    }

    // Then we feed this to the output
    memcpy(output, src, bytesToCopy);

    // Clean up dirt
    if (minBufferSize > bytesToCopy) {
        if (peakVisualizerEnabled) {
            memset(
                peakVisualizerBuffer + bytesToCopy,
                0,
                minBufferSize - bytesToCopy
            );
        }

        if (visualizerEnabled) {
            memset(
                visualizerBuffer + bytesToCopy,
                0,
                minBufferSize - bytesToCopy
            );
        }

        memset(output + bytesToCopy, 0, minBufferSize - bytesToCopy);
    }

    bufferOffset += bytesToCopy;
    emit processedSamples();

    if (playbackSecond != lastPlaybackSecond) {
        emit progressUpdate(playbackSecond);
        lastPlaybackSecond = playbackSecond;
    }

    return bytesToCopy;
}

void AudioStreamer::prepareBuffer() {
    if (rawPCM) {
        decodeRaw();
        return;
    }

    bufferOffset = 0;

    if (leftoverSize != 0) {
        memcpy(buf, leftoverBuffer.data(), leftoverSize);
        bufferOffset = leftoverSize;
        leftoverSize = 0;
    }

    while (av_read_frame(formatContext.get(), packet.get()) >= 0) {
        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet.get());
            continue;
        }

        i32 err = avcodec_send_packet(codecContext.get(), packet.get());
        av_packet_unref(packet.get());

        if (checkError(err, false, false)) {
            continue;
        }

        while (true) {
            err = avcodec_receive_frame(codecContext.get(), frame.get());

            if (err == AVERROR(EAGAIN) || checkError(err, false, false)) {
                // Read another frame
                av_frame_unref(frame.get());
                break;
            }

            processFrame();

            if (bufferOffset < minBufferSize) {
                // Read another frame
                av_frame_unref(frame.get());
                break;
            }

            leftoverSize = bufferSize % minBufferSize;
            bufferSize -= leftoverSize;

            if (leftoverSize != 0) {
                memcpy(leftoverBuffer.data(), buf + bufferSize, leftoverSize);
            }

            convertBuffer(bufferSize);
            equalizeBuffer();

            // Finish reading frames
            av_frame_unref(frame.get());
            return;
        }
    }

    buffer.clear();
    bufferSize = 0;
    leftoverSize = 0;
}

//! __restrict helps massively with native SEE/AVX optimizations here
// In this function, sampleSize can't be 1 or 3: all of
// supported encoded formats don't pack samples into signed
// 8-bit integers. They do pack samples into 24-bit integers,
// but received frames convert 24-bit integers to 32-bit ones
// beforehand. We can do a little optimization by using bit
// shifts instead of multiplying.
void AudioStreamer::processFrame() {
    const i64 timestamp = (frame->pts != AV_NOPTS_VALUE)
                              ? frame->pts
                              : frame->best_effort_timestamp;

    playbackSecond =
        u16(f64(timestamp) *
            av_q2d(formatContext->streams[audioStreamIndex]->time_base));

    const u32 size = (frame->nb_samples << (sampleSize >> 1)) * channels_;
    bufferSize = bufferOffset + size;

    if (encodedLoselessFormat) {
        buffer.resize(bufferSize);
        buf = buffer.data();
    }

    u8* __restrict const out = buf + bufferOffset;
    bufferOffset += size;

    if (planarFormat) {
        const u32 channelSampleCount = frame->nb_samples;

        for (u8 channel = 0; channel < channels_; channel++) {
            const u8* const input = frame->data[channel];

            for (u64 sample = 0; sample < channelSampleCount; sample++) {
                const u8* __restrict src =
                    input + (sample << (sampleSize >> 1));
                u8* __restrict dst = out + (((sample * channels_) + channel)
                                            << (sampleSize >> 1));
                memcpy(dst, src, sampleSize);
            }
        }
    } else {
        memcpy(out, frame->data[0], size);
    }
}

void AudioStreamer::convertBuffer(const u32 bytesRead) {
    if (codecContext->sample_fmt == F32_SAMPLE_FORMAT ||
        codecContext->sample_fmt == F32P_SAMPLE_FORMAT) {
        return;
    }

    vector<u8> floatBuffer;

    switch (sampleSize) {
        case SampleSize::U8: {
            const u8* const samples = buf;

            floatBuffer.resize(u32(bytesRead * F32_SAMPLE_SIZE));
            f32* out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, bytesRead)) {
                out[sample] = f32(samples[sample]) / (INT8_MAX + 1.0F);
            }
            break;
        }
        case SampleSize::S16: {
            const i16* const samples = ras<const i16*>(buf);

            const u32 sampleCount = bytesRead >> 1;
            const u32 floatBufferSize = sampleCount * F32_SAMPLE_SIZE;

            floatBuffer.resize(floatBufferSize);
            f32* out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                out[sample] = f32(samples[sample]) / (INT16_MAX + 1.0F);
            }
            break;
        }
        // Only the case for pcm_s24le, because in encoded formats FFmpeg
        // automatically converts 24-bit samples to 32-bit samples
        case SampleSize::S24: {
            constexpr u8 U8_BIT = SampleSize::U8 * CHAR_BIT;
            constexpr u8 I16_BIT = SampleSize::S16 * CHAR_BIT;

            const u8* const samples = buf;

            const u32 sampleCount = bytesRead / 3;
            const u32 floatBufferSize = sampleCount * F32_SAMPLE_SIZE;

            floatBuffer.resize(floatBufferSize);
            f32* out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                const u32 offset = sample * sampleSize;
                i32 value = (samples[offset]) |
                            (samples[offset + 1] << U8_BIT) |
                            (samples[offset + 2] << I16_BIT);

                if ((value & (INT24_MAX + 1)) != 0) {
                    value |= ~UINT24_MAX;
                }

                out[sample] = f32(value) / (INT24_MAX + 1.0F);
            }
            break;
        }
        case SampleSize::S32: {
            const i32* __restrict const samples = ras<const i32*>(buf);
            const u32 sampleCount = bytesRead >> 2;

            //! We can overwrite directly, because sizeof(float) == sizeof(i32)
            f32* __restrict dst = ras<f32*>(buf);

            for (const u32 sample : range(0, sampleCount)) {
                dst[sample] = f32(samples[sample]) / f32(INT32_MAX);
            }
            return;
        }
        default:
            LOG_WARN(u"Unsupported sample size: %1"_s.arg(sampleSize));
            return;
    }

    bufferSize = floatBuffer.size();

    if (encodedLoselessFormat) {
        buffer.resize(bufferSize);
        buf = buffer.data();
    }

    memcpy(buf, floatBuffer.data(), bufferSize);
}

void AudioStreamer::decodeRaw() {
    if (avio_feof(formatContext->pb) != 0) {
        bufferSize = 0;
        return;
    }

    const i32 size = (sampleSize == SampleSize::S24 ||
                      channels_ == AudioChannels::Surround51)
                         ? MIN_BUFFER_SIZE_3BYTES
                         : MIN_BUFFER_SIZE;
    const i32 bytesRead =
        avio_read(formatContext->pb, arrayBuffer.data(), size);

    if (bytesRead <= 0) {
        bufferSize = 0;
        return;
    }

    bufferSize = bytesRead;
    convertBuffer(bytesRead);
    equalizeBuffer();

    playbackSecond = avio_tell(formatContext->pb) / fileKbps;
}

void AudioStreamer::seekSecond(const u16 second) {
    const i64 timestamp =
        second == 0
            ? 0
            : av_rescale(
                  second,
                  formatContext->streams[audioStreamIndex]->time_base.den,
                  formatContext->streams[audioStreamIndex]->time_base.num
              );

    avcodec_flush_buffers(codecContext.get());

    avformat_seek_file(
        formatContext.get(),
        audioStreamIndex,
        0,
        timestamp,
        timestamp,
        AVSEEK_FLAG_ANY
    );
}

auto AudioStreamer::reset() -> bool {
    formatContext.reset();
    codecContext.reset();
    packet.reset();
    frame.reset();
    filterGraph.reset();

    buffer.clear();
    bufferSize = 0;
    leftoverSize = 0;
    bufferOffset = 0;

    return true;
}

void AudioStreamer::initializeFilters() {
    // We don't ever need to rebuild the filters, equalizer's gains are adjusted
    // through the command, if they need to be changed
    if (!settings->equalizerSettings.enabled || filterGraph != nullptr) {
        if (changedGains) {
            avfilter_graph_send_command(
                filterGraph.get(),
                "equalizer",
                "gain_entry",
                buildEqualizerArgs().c_str(),
                nullptr,
                0,
                0
            );
        }

        return;
    }

    filterGraph = createFilterGraph();
    if (filterGraph == nullptr) {
        LOG_WARN(u"Could not allocate filter graph."_s);
        filterGraph.reset();
        return;
    }

    // Get channel layout, `stereo`, `mono`, `5.1` etc.
    constexpr u8 CHANNEL_LAYOUT_SIZE = 16;
    string channelLayout = string(CHANNEL_LAYOUT_SIZE, '\0');

    i32 err = av_channel_layout_describe(
        &codecContext->ch_layout,
        channelLayout.data(),
        CHANNEL_LAYOUT_SIZE
    );
    if (checkError(err, false, true)) {
        return;
    }

    const string abufferArgs = std::format(
        "sample_fmt={}:sample_rate={}:channel_layout={}",
        F32_SAMPLE_FORMAT_NAME,
        codecContext->sample_rate,
        channelLayout
    );

    // FFT2 improves speed, it says in the docs
    // Hamming window function is suggested by ChatGPT as the best for general
    // EQ
    const string equalizerArgs = std::format(
        "gain_entry='{}':fft2=on:wfunc=hamming",
        buildEqualizerArgs()
    );

    const string aformatArgs = std::format(
        "sample_fmts={}:sample_rates={}:channel_layouts={}",
        F32_SAMPLE_FORMAT_NAME,
        codecContext->sample_rate,
        channelLayout
    );

    // Order DOES matter!
    // `abuffer` always must be first, and `aformat` with `abuffersink` last.
    const array<Filter, 5> filters = {
        Filter{ .filter = "abuffer",
                .name = "src",
                .args = abufferArgs.c_str(),
                .context = &abufferContext },
        { .filter = "firequalizer",
          .name = "equalizer",
          .args = equalizerArgs.c_str(),
          .context = &equalizerContext },
        { .filter = "alimiter",
          .name = "limiter",
          .args = nullptr,
          .context = &limiterContext },
        { .filter = "aformat",
          .name = "aformat",
          .args = aformatArgs.c_str(),
          .context = &aformatContext },
        { .filter = "abuffersink",
          .name = "sink",
          .args = nullptr,
          .context = &abuffersinkContext },
    };

    // Create filters
    for (const auto& filter : filters) {
        const AVFilter* avfilter = avfilter_get_by_name(filter.filter);

        err = avfilter_graph_create_filter(
            filter.context,
            avfilter,
            filter.name,
            filter.args,
            nullptr,
            filterGraph.get()
        );

        if (checkError(err, false, true)) {
            return;
        }
    }

    // Connect filters
    for (const u8 idx : range(0, filters.size() - 1)) {
        err = avfilter_link(
            *filters[idx].context,
            0,
            *filters[idx + 1].context,
            0
        );

        if (checkError(err, false, true)) {
            return;
        }
    }

    // Check the validity of filter graph
    err = avfilter_graph_config(filterGraph.get(), nullptr);
    if (checkError(err, false, true)) {
        return;
    }
}

auto AudioStreamer::buildEqualizerArgs() const -> string {
    string args;

    constexpr u8 stringSize = sizeof("entry(,);") - 1;
    constexpr u8 maxFrequencyLength = 5;
    constexpr u8 maxGainLength = 2;

    args.reserve(
        usize(eqSettings.bandCount) *
        (stringSize + maxFrequencyLength + maxGainLength)
    );

    for (const auto [freq, gain] : views::take(
             views::zip(
                 span(eqSettings.frequencies, eqSettings.bandCount),
                 eqSettings.gains
             ),
             eqSettings.bandCount
         )) {
        args += std::format("entry({},{});", freq, gain);
    }

    args.pop_back();  // Remove last semicolon
    return args;
}

void AudioStreamer::equalizeBuffer() {
    initializeFilters();

    if (!settings->equalizerSettings.enabled || filterGraph == nullptr ||
        ranges::all_of(eqSettings.gains, [](const i8 gain) -> bool {
        return gain == 0;
    })) {
        return;
    }

    AVFrame* unfilteredFrame = av_frame_alloc();
    unfilteredFrame->nb_samples =
        i32(bufferSize / i32(F32_SAMPLE_SIZE * channels_));
    unfilteredFrame->format = F32_SAMPLE_FORMAT;
    unfilteredFrame->ch_layout = codecContext->ch_layout;
    unfilteredFrame->sample_rate = codecContext->sample_rate;

    av_frame_get_buffer(unfilteredFrame, 0);
    memcpy(unfilteredFrame->data[0], buf, bufferSize);

    if (!settings->equalizerSettings.enabled || filterGraph == nullptr ||
        ranges::all_of(eqSettings.gains, [](const i8 gain) -> bool {
        return gain == 0;
    })) {
        return;
    }

    i32 err = av_buffersrc_add_frame(abufferContext, unfilteredFrame);
    if (checkError(err, false, false)) {
        av_frame_free(&unfilteredFrame);
        return;
    }

    AVFrame* filteredFrame = av_frame_alloc();
    err = av_buffersink_get_frame(abuffersinkContext, filteredFrame);

    if (checkError(err, false, false)) {
        av_frame_free(&filteredFrame);
        return;
    }

    memcpy(buf, filteredFrame->data[0], bufferSize);
    av_frame_free(&filteredFrame);
    av_frame_free(&unfilteredFrame);
}
