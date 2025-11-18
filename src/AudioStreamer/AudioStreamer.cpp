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

AudioStreamer::AudioStreamer(
    shared_ptr<Settings> settings_,
    QObject* const parent
) :
    QObject(parent),
    settings(std::move(settings_)),
    eqSettings(settings->equalizer) {
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
    audioStream = fCtxPtr->streams[audioStreamIndex];

    codecContext = createCodecContext(codec);
    avcodec_parameters_to_context(codecContext.get(), audioStream->codecpar);
    err = avcodec_open2(codecContext.get(), codec, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    const AVChannelLayout& channelLayout = codecContext->ch_layout;
    channels_ = AudioChannels(codecContext->ch_layout.nb_channels);
    sampleRate_ = codecContext->sample_rate;

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
        fileKbps = sampleRate_ * u8(channels_) * u8(sampleSize);

        rawBufferSize = (sampleSize == SampleSize::S24 ||
                         channels_ == AudioChannels::Surround51)
                            ? MIN_BUFFER_SIZE_3BYTES
                            : MIN_BUFFER_SIZE;
    } else {
        planarFormat = bool(av_sample_fmt_is_planar(codecContext->sample_fmt));

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

    preparedBuffers = 0;

    for (auto& buffer : buffers) {
        buffer.buf.resize(0);
        buffer.buf.shrink_to_fit();
        buffer.buf.reserve(UINT16_MAX + 1);
        prepareBuffer();
    }

    initializeFilters(true);
}

void AudioStreamer::prepareBuffer() {
    if (preparedBuffers == BUFFERS_QUEUE_SIZE) {
        preparedBuffers = 0;
    }

    buffer = &buffers[preparedBuffers++];

    if (rawPCM) {
        buffer->buf.resize(rawBufferSize);
        decodeRaw();
        return;
    }

    bufferOffset = 0;

    if (leftoverSize != 0) {
        buffer->buf.resize(leftoverSize);
        memcpy(buffer->buf.data(), leftoverBuffer.data(), leftoverSize);
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

            if (leftoverSize != 0) {
                bufferSize -= leftoverSize;

                memcpy(
                    leftoverBuffer.data(),
                    buffer->buf.data() + bufferSize,
                    leftoverSize
                );

                buffer->buf.resize(bufferSize);
            }

            convertBuffer();
            equalizeBuffer();

            // Finish reading frames
            av_frame_unref(frame.get());
            return;
        }
    }

    bufferSize = 0;
    leftoverSize = 0;
    buffer->buf.resize(0);
}

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

    buffer->second = u16(f64(timestamp) * av_q2d(audioStream->time_base));

    const u32 size =
        (frame->nb_samples << (u8(sampleSize) >> 1)) * u8(channels_);

    bufferSize = size + bufferOffset;
    buffer->buf.resize(bufferSize);

    u8* const out = buffer->buf.data() + bufferOffset;
    bufferOffset += size;

    const u8 halfSampleSize = u8(sampleSize) >> 1;

    if (planarFormat) {
        const u32 channelSampleCount = frame->nb_samples;

        for (const u8 channel : range(0, u8(channels_))) {
            const u8* const input = frame->data[channel];

            for (const u64 sample : range(0, channelSampleCount)) {
                const u8* const src = input + (sample << halfSampleSize);
                u8* const dst = out + (((sample * u8(channels_)) + channel)
                                       << halfSampleSize);
                memcpy(dst, src, u8(sampleSize));
            }
        }
    } else {
        memcpy(out, frame->data[0], size);
    }
}

void AudioStreamer::convertBuffer() {
    if (codecContext->sample_fmt == F32_SAMPLE_FORMAT ||
        codecContext->sample_fmt == F32P_SAMPLE_FORMAT) {
        return;
    }

    vector<u8> floatBuffer;

    switch (sampleSize) {
        // Currently unused, because pcm_u8 is unsupported
        case SampleSize::U8: {
            const u8* const samples = buffer->buf.data();

            floatBuffer.resize(u32(bufferSize * F32_SAMPLE_SIZE));
            f32* const out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, bufferSize)) {
                out[sample] = f32(samples[sample]) / (INT8_MAX + 1.0F);
            }
            break;
        }
        case SampleSize::S16: {
            const i16* const samples = ras<const i16*>(buffer->buf.data());

            const u32 sampleCount = bufferSize >> 1;
            const u32 floatBufferSize = sampleCount * F32_SAMPLE_SIZE;

            floatBuffer.resize(floatBufferSize);
            f32* const out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                out[sample] = f32(samples[sample]) / (INT16_MAX + 1.0F);
            }
            break;
        }
        // Only the case for pcm_s24le, because in encoded formats FFmpeg
        // automatically converts 24-bit samples to 32-bit samples
        case SampleSize::S24: {
            constexpr u8 U8_BIT = u8(SampleSize::U8) * CHAR_BIT;
            constexpr u8 I16_BIT = u8(SampleSize::S16) * CHAR_BIT;

            const u8* const samples = buffer->buf.data();

            const u32 sampleCount = bufferSize / 3;
            const u32 floatBufferSize = sampleCount * F32_SAMPLE_SIZE;

            floatBuffer.resize(floatBufferSize);
            f32* const out = ras<f32*>(floatBuffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                const u32 offset = sample * u8(sampleSize);
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
            const i32* const samples = ras<const i32*>(buffer->buf.data());
            const u32 sampleCount = bufferSize >> 2;

            //! We can overwrite directly, because sizeof(f32) == sizeof(i32)
            f32* const out = ras<f32*>(buffer->buf.data());

            for (const u32 sample : range(0, sampleCount)) {
                out[sample] = f32(samples[sample]) / f32(INT32_MAX);
            }

            buffer->buf.resize(bufferSize);
            return;
        }
        default:
            LOG_WARN(
                u"Unsupported sample size: "_s + QString::number(u8(sampleSize))
            );
            return;
    }

    bufferSize = floatBuffer.size();
    buffer->buf.resize(bufferSize);

    memcpy(buffer->buf.data(), floatBuffer.data(), bufferSize);
}

void AudioStreamer::decodeRaw() {
    if (avio_feof(formatContext->pb) != 0) {
        bufferSize = 0;
        buffer->buf.resize(0);
        return;
    }

    const i32 bytesRead =
        avio_read(formatContext->pb, buffer->buf.data(), rawBufferSize);

    if (bytesRead <= 0) {
        bufferSize = 0;
        buffer->buf.resize(0);
        return;
    }

    buffer->buf.resize(bytesRead);
    bufferSize = bytesRead;

    convertBuffer();
    equalizeBuffer();

    buffer->second = avio_tell(formatContext->pb) / fileKbps;
}

void AudioStreamer::seekSecond(const u16 second) {
    const i64 timestamp = second == 0 ? 0
                                      : av_rescale(
                                            second,
                                            audioStream->time_base.den,
                                            audioStream->time_base.num
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

    bufferSize = 0;
    leftoverSize = 0;
    bufferOffset = 0;

    return true;
}

void AudioStreamer::initializeFilters(const bool force) {
    // We don't ever need to rebuild the filters, equalizer's gains are adjusted
    // through the command, if they need to be changed
    if (!force && (!settings->equalizer.enabled || filterGraph != nullptr)) {
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
        "gain_entry='{}':fft2=on:wfunc=hamming:accuracy=10",
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
        const AVFilter* const avfilter = avfilter_get_by_name(filter.filter);

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
                 span(eqSettings.frequencies, usize(eqSettings.bandCount)),
                 eqSettings.gains
             ),
             u8(eqSettings.bandCount)
         )) {
        args += std::format("entry({},{});", freq, gain);
    }

    args.pop_back();  // Remove last semicolon
    return args;
}

void AudioStreamer::equalizeBuffer() {
    initializeFilters();

    if (!settings->equalizer.enabled || filterGraph == nullptr ||
        ranges::all_of(
            span<i8>(eqSettings.gains.data(), usize(eqSettings.bandCount)),
            [](const i8 gain) -> bool { return gain == 0; }
        )) {
        return;
    }

    AVFrame* unfilteredFrame = av_frame_alloc();
    unfilteredFrame->nb_samples =
        i32(bufferSize / i32(F32_SAMPLE_SIZE * u8(channels_)));
    unfilteredFrame->format = F32_SAMPLE_FORMAT;
    unfilteredFrame->ch_layout = codecContext->ch_layout;
    unfilteredFrame->sample_rate = codecContext->sample_rate;

    av_frame_get_buffer(unfilteredFrame, 0);
    memcpy(unfilteredFrame->data[0], buffer->buf.data(), bufferSize);

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

    memcpy(buffer->buf.data(), filteredFrame->data[0], bufferSize);
    av_frame_free(&filteredFrame);
    av_frame_free(&unfilteredFrame);
}
