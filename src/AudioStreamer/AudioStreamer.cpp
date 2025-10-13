#include "AudioStreamer.hpp"

#include "Aliases.hpp"
#include "Constants.hpp"

struct Filter {
    cstr filter;
    cstr name;
    cstr args;
    AVFilterContext** context;
};

AudioStreamer::AudioStreamer(QObject* parent) : QIODevice(parent) {
    format_.setSampleFormat(QAudioFormat::Float);

#ifdef DEBUG_BUILD
    av_log_set_level(AV_LOG_VERBOSE);
#elifdef RELEASE_BUILD
    av_log_set_level(AV_LOG_QUIET);
#endif
}

void AudioStreamer::start(const QString& path) {
    reset();

    const string pathString = path.toStdString();

    AVFormatContext* fCtxPtr = formatContext.get();

    i32 err;
    err = avformat_open_input(&fCtxPtr, pathString.c_str(), nullptr, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    formatContext.reset(fCtxPtr);
    const i64 headerLength = avio_tell(formatContext->pb);

    err = avformat_find_stream_info(fCtxPtr, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    const AVCodec* codec;
    err = av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

    if (checkError(err, true, false)) {
        return;
    }

    audioStreamIndex = u8(err);

    const AVStream* audioStream = fCtxPtr->streams[audioStreamIndex];
    codecContext = createCodecContext(codec);

    AVCodecContext* cCtxPtr = codecContext.get();
    avcodec_parameters_to_context(cCtxPtr, audioStream->codecpar);

    err = avcodec_open2(cCtxPtr, codec, nullptr);

    if (checkError(err, true, false)) {
        return;
    }

    const AVChannelLayout& channelLayout = codecContext->ch_layout;
    const u8 channelCount = codecContext->ch_layout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;

    format_.setSampleRate(sampleRate);
    format_.setChannelCount(channelCount);

    const string_view formatName = cCtxPtr->codec->name;
    rawPCM = formatName.starts_with("pcm");

    if (rawPCM) {
        // Ensure that we are at the very start of the data,
        // when reading raw PCM
        avio_seek(formatContext->pb, headerLength, SEEK_SET);

        inputSampleSize =
            formatName.contains("24")
                ? INT24_SIZE
                : av_get_bytes_per_sample(codecContext->sample_fmt);
        fileKbps = sampleRate * channelCount * inputSampleSize;
    } else {
        // We only need packets and frames when decoding the encoded data
        packet = createPacket();
        frame = createFrame();

        // We only use FFmpeg's swresample when decoding the encoded data, in
        // raw formats we handle it manually
        ::SwrContext* swrCtxPtr = swrContext.get();

        err = swr_alloc_set_opts2(
            &swrCtxPtr,
            &channelLayout,
            F32_SAMPLE_FORMAT,
            sampleRate,
            &channelLayout,
            codecContext->sample_fmt,
            sampleRate,
            0,
            nullptr
        );

        if (checkError(err, true, false)) {
            return;
        }

        swrContext.reset(swrCtxPtr);
        swr_init(swrCtxPtr);
    }

    open(QIODevice::ReadOnly);
}

auto AudioStreamer::processFrame(i64 totalRead, str buf) -> i64 {
    const f64 timestamp =
        f64((frame->pts != AV_NOPTS_VALUE) ? frame->pts
                                           : frame->best_effort_timestamp);

    playbackSecond =
        u16(timestamp *
            av_q2d(formatContext->streams[audioStreamIndex]->time_base));

    convertFrame();
    equalizeFrame();

    const u32 bufferSize =
        frame->nb_samples * F32_SAMPLE_SIZE * frame->ch_layout.nb_channels;
    memcpy(buf + totalRead, frame->data[0], bufferSize);
    return bufferSize;
}

auto AudioStreamer::buildEqualizerArgs() const -> string {
    string args;

    constexpr u8 stringSize = sizeof("entry(,);") - 1;
    constexpr u8 maxFrequencyLength = 5;
    constexpr u8 maxGainLength = 2;

    args.reserve(
        usize(bandCount) * (stringSize + maxFrequencyLength + maxGainLength)
    );

    for (const auto [freq, gain] :
         views::take(views::zip(frequencies_, gains_), bandCount)) {
        args += std::format("entry({},{});", freq, gain);
    }

    args.pop_back();  // Remove last semicolon
    return args;
}

void AudioStreamer::initializeFilters() {
    // We don't ever need to rebuild the filters, equalizer's gains are adjusted
    // through the command, if they need to be changed
    if (!equalizerEnabled_ || filterGraph != nullptr) {
        if (changedBands) {
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

void AudioStreamer::equalizeBuffer(vector<u8>& buffer) {
    if (!equalizerEnabled_ || filterGraph == nullptr ||
        ranges::all_of(gains_, [](const i8 gain) { return gain == 0; })) {
        return;
    }

    const u8 channelCount = codecContext->ch_layout.nb_channels;
    const u32 bufferSize = buffer.size();

    frame = createFrame();
    frame->nb_samples = i32(bufferSize / (F32_SAMPLE_SIZE * channelCount));
    frame->format = F32_SAMPLE_FORMAT;
    frame->ch_layout = codecContext->ch_layout;
    frame->sample_rate = codecContext->sample_rate;

    av_frame_get_buffer(frame.get(), 0);
    memcpy(frame->data[0], buffer.data(), bufferSize);

    equalizeFrame();

    memcpy(buffer.data(), frame->data[0], bufferSize);
    frame.reset();
}

void AudioStreamer::equalizeFrame() {
    if (!equalizerEnabled_ || filterGraph == nullptr ||
        ranges::all_of(gains_, [](const i8 gain) { return gain == 0; })) {
        return;
    }

    i32 err = av_buffersrc_add_frame(abufferContext, frame.get());
    if (checkError(err, false, false)) {
        return;
    }

    AVFrame* filteredFrame = av_frame_alloc();
    err = av_buffersink_get_frame(abuffersinkContext, filteredFrame);

    if (checkError(err, false, false)) {
        av_frame_free(&filteredFrame);
        return;
    }

    frame.reset(filteredFrame);
}

void AudioStreamer::convertBuffer(vector<u8>& buffer, const u32 bytesRead) {
    if (codecContext->sample_fmt == F32_SAMPLE_FORMAT) {
        return;
    }

    const u8 channelCount = codecContext->ch_layout.nb_channels;
    const u32 sampleCount = bytesRead / inputSampleSize;
    const u32 floatBufferSize = sampleCount * F32_SAMPLE_SIZE;

    vector<f32> outBuffer;
    outBuffer.reserve(floatBufferSize);

    switch (inputSampleSize) {
        case sizeof(i8): {
            const i8* samples = ras<const i8*>(buffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                outBuffer.emplace_back(
                    f32(samples[sample]) / (INT8_MAX + 1.0F)
                );
            }
            break;
        }
        case sizeof(i16): {
            const i16* samples = ras<const i16*>(buffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                outBuffer.emplace_back(
                    f32(samples[sample]) / (INT16_MAX + 1.0F)
                );
            }
            break;
        }
        case sizeof(i8) + sizeof(i16): {
            constexpr u8 I8_BIT = sizeof(i8) * CHAR_BIT;
            constexpr u8 I16_BIT = sizeof(i16) * CHAR_BIT;

            const u8* samples = buffer.data();

            for (const u32 sample : range(0, sampleCount)) {
                const u32 offset = sample * inputSampleSize;
                i32 value = (samples[offset]) |
                            (samples[offset + 1] << I8_BIT) |
                            (samples[offset + 2] << I16_BIT);

                if ((value & (INT24_MAX + 1)) != 0) {
                    value |= ~UINT24_MAX;
                }

                outBuffer.emplace_back(f32(value) / (INT24_MAX + 1.0F));
            }
            break;
        }
        case sizeof(i32): {
            const i32* samples = ras<const i32*>(buffer.data());

            for (const u32 sample : range(0, sampleCount)) {
                outBuffer.emplace_back(f32(samples[sample]) / f32(INT32_MAX));
            }
            break;
        }
        default:
            LOG_WARN(u"Unsupported sample size: %1"_s.arg(inputSampleSize));
            break;
    }

    buffer.resize(floatBufferSize);
    memcpy(buffer.data(), outBuffer.data(), floatBufferSize);
}

void AudioStreamer::convertFrame() {
    if (codecContext->sample_fmt == F32_SAMPLE_FORMAT) {
        return;
    }

    AVFrame* newFrame = av_frame_alloc();
    newFrame->ch_layout = codecContext->ch_layout;
    newFrame->sample_rate = codecContext->sample_rate;
    newFrame->format = F32_SAMPLE_FORMAT;

    swr_convert_frame(swrContext.get(), newFrame, frame.get());
    frame.reset(newFrame);
}

auto AudioStreamer::decodeRaw(str buf) -> i64 {
    if (avio_feof(formatContext->pb) != 0) {
        return 0;
    }

    const i32 size =
        i32(inputSampleSize == sizeof(i8) + sizeof(i16) ? MIN_BUFFER_SIZE_I24
                                                        : MIN_BUFFER_SIZE);
    vector<u8> buffer;
    buffer.resize(size);

    const i32 bytesRead = avio_read(formatContext->pb, buffer.data(), size);

    if (bytesRead <= 0) {
        return 0;
    }

    convertBuffer(buffer, bytesRead);
    equalizeBuffer(buffer);

    memcpy(buf, buffer.data(), size);

    playbackSecond = avio_tell(formatContext->pb) / fileKbps;
    return bytesRead;
}

auto AudioStreamer::prepareBuffer(str buf) -> i64 {
    if (rawPCM) {
        return decodeRaw(buf);
    }

    i64 totalRead = 0;

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
                av_frame_unref(frame.get());
                break;
            }

            const i64 read = processFrame(totalRead, buf);
            totalRead += read;

            if (totalRead < MIN_BUFFER_SIZE) {
                av_frame_unref(frame.get());
                break;
            }

            return totalRead;
        }
    }

    return 0;
}

auto AudioStreamer::readData(str data, const qi64 /* size */) -> qi64 {
    const i64 read = prepareBuffer(data);

    if (read == 0) {
        emit streamEnded();
    } else {
        visualizerBuffer->resize(read);
        memcpy(visualizerBuffer->data(), data, read);
        emit buildPeaks(codecContext->sample_rate);
    }

    if (playbackSecond != lastPlaybackSecond) {
        emit progressUpdate(playbackSecond);
        lastPlaybackSecond = playbackSecond;
    }

    return read;
}

auto AudioStreamer::reset() -> bool {
    QIODevice::close();

    formatContext.reset();
    codecContext.reset();
    swrContext.reset();
    packet.reset();
    frame.reset();
    filterGraph.reset();

    return true;
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

void AudioStreamer::setBandCount(const u8 bands) {
    bandCount = bands;

    switch (bands) {
        case THREE_BANDS:
            memcpy(
                frequencies_.data(),
                THREE_BAND_FREQUENCIES.data(),
                u16(THREE_BANDS * F32_SAMPLE_SIZE)
            );
            break;
        case FIVE_BANDS:
            memcpy(
                frequencies_.data(),
                FIVE_BAND_FREQUENCIES.data(),
                u16(FIVE_BANDS * F32_SAMPLE_SIZE)
            );
            break;
        case TEN_BANDS:
            memcpy(
                frequencies_.data(),
                TEN_BAND_FREQUENCIES.data(),
                u16(TEN_BANDS * F32_SAMPLE_SIZE)
            );
            break;
        case EIGHTEEN_BANDS:
            memcpy(
                frequencies_.data(),
                EIGHTEEN_BAND_FREQUENCIES.data(),
                u16(EIGHTEEN_BANDS * F32_SAMPLE_SIZE)
            );
            break;
        case THIRTY_BANDS:
            memcpy(
                frequencies_.data(),
                THIRTY_BAND_FREQUENCIES.data(),
                u16(THIRTY_BANDS * F32_SAMPLE_SIZE)
            );
            break;
        default:
            break;
    }

    gains_.fill(0);
}
