#include "audiostreamer.hpp"

#include "constants.hpp"

#include <libavfilter/avfilter.h>

constexpr u16 MIN_BUFFER_SIZE = 4096;

AudioStreamer::AudioStreamer(QObject* parent) : QIODevice(parent) {
    format_.setSampleFormat(QAudioFormat::Float);
}

void AudioStreamer::start(const QString& path) {
    reset();

    AVFormatContext* formatContextPtr = formatContext.get();

    i32 err;
    err = avformat_open_input(
        &formatContextPtr,
        path.toUtf8().constData(),
        nullptr,
        nullptr
    );

    if (err < 0) {
        printErr(err);
        reset();
        return;
    }

    formatContext.reset(formatContextPtr);
    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        printErr(err);
        reset();
        return;
    }

    const AVCodec* codec;
    audioStreamIndex = as<i8>(av_find_best_stream(
        formatContextPtr,
        AVMEDIA_TYPE_AUDIO,
        -1,
        -1,
        &codec,
        0
    ));

    if (audioStreamIndex < 0) {
        printErr(audioStreamIndex);
        reset();
        return;
    }

    const AVStream* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = createCodecContext(codec);

    AVCodecContext* CodecContext = codecContext.get();
    avcodec_parameters_to_context(CodecContext, audioStream->codecpar);

    err = avcodec_open2(CodecContext, codec, nullptr);

    if (err < 0) {
        printErr(err);
        reset();
        return;
    }

    const AVChannelLayout channelLayout = codecContext->ch_layout;
    const u8 channelNumber = channelLayout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;

    ::SwrContext* swrContextPtr = swrContext.get();

    err = swr_alloc_set_opts2(
        &swrContextPtr,
        &channelLayout,
        AV_SAMPLE_FMT_FLT,
        sampleRate,
        &channelLayout,
        codecContext->sample_fmt,
        sampleRate,
        0,
        nullptr
    );

    if (err < 0) {
        printErr(err);
        reset();
        return;
    }

    swrContext.reset(swrContextPtr);
    swr_init(swrContextPtr);

    packet = createPacket();
    frame = createFrame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;

    format_.setSampleRate(sampleRate);
    format_.setChannelCount(channelNumber);

    formatName = CodecContext->codec->name;
    inputSampleSize = av_get_bytes_per_sample(codecContext->sample_fmt);
    bytesPerSecond =
        as<u32>(
            codecContext->sample_rate * codecContext->ch_layout.nb_channels
        ) *
        inputSampleSize;

    // Ensure that we are at the very start of the data,
    // when reading raw PCM
    if (formatName.starts_with("pcm")) {
        avio_seek(formatContext->pb, 0, SEEK_SET);
    }

    initializeFilters();
    prepareBuffer();

    open(QIODevice::ReadOnly);
}

auto AudioStreamer::processFrame() -> bool {
    const f64 timestamp = as<f64>(
        (frame->pts != AV_NOPTS_VALUE) ? frame->pts
                                       : frame->best_effort_timestamp
    );

    playbackSecond = as<u16>(
        timestamp * av_q2d(formatContext->streams[audioStreamIndex]->time_base)
    );

    convertFrame();
    equalizeFrame();

    const u32 bufferSize =
        frame->nb_samples * SAMPLE_SIZE * frame->ch_layout.nb_channels;

    buffer.append(ras<cstr>(frame->data[0]), bufferSize);

    nextBufferSize = buffer.size();
    return nextBufferSize >= MIN_BUFFER_SIZE;
}

auto AudioStreamer::buildEqualizerArgs(const bool change) -> string {
    u16 width;

    switch (bandCount) {
        case THREE_BANDS:
            width = THREE_BANDS_WIDTH;
            break;
        case FIVE_BANDS:
            width = FIVE_BANDS_WIDTH;
            break;
        case TEN_BANDS:
            width = TEN_BANDS_WIDTH;
            break;
        case EIGHTEEN_BANDS:
            width = EIGHTEEN_BANDS_WIDTH;
            break;
        case THIRTY_BANDS:
            width = THIRTY_BANDS_WIDTH;
            break;
        default:
            break;
    }

    const u8 channelCount = codecContext->ch_layout.nb_channels;
    string equalizerArgs;

    for (const u8 idx : range(0, bandCount)) {
        if (idx > 0) {
            equalizerArgs += '|';
        }

        for (const u8 channel : range(0, channelCount)) {
            if (channel > 0) {
                equalizerArgs += '|';
            }

            equalizerArgs += std::format(
                "c{} f={} w={} g={} t=1",
                channel,
                frequencies_[idx],
                width,
                gains_[idx]
            );
        }
    }

    return equalizerArgs;
}

void AudioStreamer::initializeFilters() {
    if (!equalizerEnabled_) {
        return;
    }

    if (!bandCountChanged) {
        u16 width;

        switch (bandCount) {
            case THREE_BANDS:
                width = THREE_BANDS_WIDTH;
                break;
            case FIVE_BANDS:
                width = FIVE_BANDS_WIDTH;
                break;
            case TEN_BANDS:
                width = TEN_BANDS_WIDTH;
                break;
            case EIGHTEEN_BANDS:
                width = EIGHTEEN_BANDS_WIDTH;
                break;
            case THIRTY_BANDS:
                width = THIRTY_BANDS_WIDTH;
                break;
            default:
                break;
        }

        const u8 channelCount = codecContext->ch_layout.nb_channels;

        for (u16 band = 0; band < bandCount; band++) {
            if (!changedBands[band]) {
                continue;
            }

            for (u8 channel = 0; channel < channelCount; channel++) {
                const u16 idx = (band * channelCount) + channel;

                const string equalizerArgs = std::format(
                    "{}|f={}|w={}|g={}",
                    idx,
                    frequencies_[band],
                    width,
                    gains_[band]
                );

                avfilter_graph_send_command(
                    filterGraph.get(),
                    "equalizer",
                    "change",
                    equalizerArgs.c_str(),
                    nullptr,
                    0,
                    0
                );
            }

            changedBands[band] = false;
        }

        return;
    }

    bandCountChanged = false;

    filterGraph = createFilterGraph();
    if (filterGraph == nullptr) {
        qWarning() << "Could not allocate filter graph.";
        filterGraph.reset();
        return;
    }

    const AVFilter* abuffer = avfilter_get_by_name("abuffer");
    const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
    const AVFilter* aformat = avfilter_get_by_name("aformat");
    const AVFilter* equalizer = avfilter_get_by_name("anequalizer");
    const AVFilter* normalizer = avfilter_get_by_name("alimiter");

    constexpr u8 CHANNEL_LAYOUT_SIZE = 16;
    string channelLayout(CHANNEL_LAYOUT_SIZE, '\0');

    i32 err = av_channel_layout_describe(
        &codecContext->ch_layout,
        channelLayout.data(),
        CHANNEL_LAYOUT_SIZE
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    const string abufferArgs = std::format(
        "sample_fmt={}:sample_rate={}:channel_layout={}",
        av_get_sample_fmt_name(AV_SAMPLE_FMT_FLT),
        codecContext->sample_rate,
        channelLayout
    );

    err = avfilter_graph_create_filter(
        &abufferContext,
        abuffer,
        "src",
        abufferArgs.c_str(),
        nullptr,
        filterGraph.get()
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    const string equalizerArgs = buildEqualizerArgs(false);

    qDebug() << equalizerArgs;

    err = avfilter_graph_create_filter(
        &equalizerContext,
        equalizer,
        "equalizer",
        equalizerArgs.c_str(),
        nullptr,
        filterGraph.get()
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_graph_create_filter(
        &normalizerContext,
        normalizer,
        "normalizer",
        "",
        nullptr,
        filterGraph.get()
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    const string aformatArgs = std::format(
        "sample_fmts={}:sample_rates={}:channel_layouts={}",
        av_get_sample_fmt_name(AV_SAMPLE_FMT_FLT),
        codecContext->sample_rate,
        channelLayout
    );

    err = avfilter_graph_create_filter(
        &aformatContext,
        aformat,
        "aformat",
        aformatArgs.c_str(),
        nullptr,
        filterGraph.get()
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_graph_create_filter(
        &abuffersinkContext,
        abuffersink,
        "sink",
        nullptr,
        nullptr,
        filterGraph.get()
    );
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_link(abufferContext, 0, equalizerContext, 0);
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_link(equalizerContext, 0, normalizerContext, 0);
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_link(normalizerContext, 0, aformatContext, 0);
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_link(aformatContext, 0, abuffersinkContext, 0);
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }

    err = avfilter_graph_config(filterGraph.get(), nullptr);
    if (err < 0) {
        printErr(err);
        filterGraph.reset();
        return;
    }
}

void AudioStreamer::equalizeFrame() {
    if (!equalizerEnabled_ || filterGraph == nullptr ||
        ranges::all_of(gains_, [](const i8 gain) { return gain == 0; })) {
        return;
    }

    i32 err = av_buffersrc_add_frame(abufferContext, frame.get());
    if (err < 0) {
        printErr(err);
        return;
    }

    AVFrame* filteredFrame = av_frame_alloc();
    err = av_buffersink_get_frame(abuffersinkContext, filteredFrame);

    if (err < 0) {
        printErr(err);
        av_frame_free(&filteredFrame);
        return;
    }

    frame.reset(filteredFrame);
}

void AudioStreamer::convertFrame() {
    if (frame == nullptr) {
        frame = createFrame();
        frame->ch_layout = codecContext->ch_layout;
        frame->sample_rate = codecContext->sample_rate;
        frame->format = codecContext->sample_fmt;
        frame->nb_samples = as<i32>(
            buffer.size() / av_get_bytes_per_sample(codecContext->sample_fmt)
        );
        frame->data[0] = ras<u8*>(buffer.data());
    }

    if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLT) {
        return;
    }

    AVFrame* newFrame = av_frame_alloc();
    newFrame->ch_layout = codecContext->ch_layout;
    newFrame->sample_rate = codecContext->sample_rate;
    newFrame->format = AV_SAMPLE_FMT_FLT;

    swr_convert_frame(swrContext.get(), newFrame, frame.get());
    frame.reset(newFrame);
}

void AudioStreamer::decodeRaw() {
    buffer.resize(MIN_BUFFER_SIZE);

    if (avio_feof(formatContext->pb) != 0) {
        return;
    }

    const i32 bytesRead =
        avio_read(formatContext->pb, ras<u8*>(buffer.data()), MIN_BUFFER_SIZE);

    if (bytesRead <= 0) {
        return;
    }

    totalBytesRead += bytesRead;
    playbackSecond = as<u16>(totalBytesRead / bytesPerSecond);

    convertFrame();
    equalizeFrame();

    memcpy(buffer.data(), frame->data[0], buffer.size());
    nextBufferSize = buffer.size();

    frame.reset();
}

void AudioStreamer::prepareBuffer() {
    nextBufferSize = 0;
    buffer.clear();

    if (formatName.starts_with("pcm")) {
        decodeRaw();
        return;
    }

    while (av_read_frame(formatContext.get(), packet.get()) >= 0) {
        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet.get());
            continue;
        }

        i32 err = avcodec_send_packet(codecContext.get(), packet.get());
        av_packet_unref(packet.get());

        if (err < 0) {
            printErr(err);
            continue;
        }

        while (true) {
            err = avcodec_receive_frame(codecContext.get(), frame.get());

            if (err < 0) {
                av_frame_unref(frame.get());
                printErr(err);
                break;
            }

            if (processFrame()) {
                av_frame_unref(frame.get());
                return;
            }
        }
    }
}

auto AudioStreamer::readData(str data, const qi64 /* size */) -> qi64 {
    const u32 bufferSize = buffer.size();
    memcpy(data, buffer.constData(), bufferSize);

    emit progressUpdate(playbackSecond);

    initializeFilters();
    prepareBuffer();

    if (nextBufferSize == 0) {
        emit streamEnded();
    }

    return bufferSize;
}

auto AudioStreamer::reset() -> bool {
    QIODevice::close();

    formatContext.reset();
    codecContext.reset();
    swrContext.reset();
    packet.reset();
    frame.reset();
    filterGraph.reset();

    buffer.clear();

    nextBufferSize = 0;
    audioStreamIndex = 0;
    secondsDuration = 0;
    playbackSecond = 0;

    totalBytesRead = 0;
    inputSampleSize = 0;
    bytesPerSecond = 0;

    formatName = "";

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

    if (formatName.starts_with("pcm") && (formatContext->pb != nullptr)) {
        totalBytesRead = avio_tell(formatContext->pb);
    }

    initializeFilters();
    prepareBuffer();
}

void AudioStreamer::setBandCount(const u8 bands) {
    bandCount = bands;

    switch (bands) {
        case THREE_BANDS:
            memcpy(
                frequencies_.data(),
                THREE_BAND_FREQUENCIES.data(),
                THREE_BANDS * SAMPLE_SIZE
            );
            break;
        case FIVE_BANDS:
            memcpy(
                frequencies_.data(),
                FIVE_BAND_FREQUENCIES.data(),
                FIVE_BANDS * SAMPLE_SIZE
            );
            break;
        case TEN_BANDS:
            memcpy(
                frequencies_.data(),
                TEN_BAND_FREQUENCIES.data(),
                TEN_BANDS * SAMPLE_SIZE
            );
            break;
        case EIGHTEEN_BANDS:
            memcpy(
                frequencies_.data(),
                EIGHTEEN_BAND_FREQUENCIES.data(),
                EIGHTEEN_BANDS * SAMPLE_SIZE
            );
            break;
        case THIRTY_BANDS:
            memcpy(
                frequencies_.data(),
                THIRTY_BAND_FREQUENCIES.data(),
                THIRTY_BANDS * SAMPLE_SIZE
            );
            break;
        default:
            break;
    }

    gains_.fill(0);
    changedBands.fill(false);
    bandCountChanged = true;
}
