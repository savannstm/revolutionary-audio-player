#include "audiostreamer.hpp"

#include <libavutil/avutil.h>

#include <QDebug>

AudioStreamer::AudioStreamer(QObject* parent) : QIODevice(parent) {
    format_.setSampleFormat(QAudioFormat::Float);
}

void AudioStreamer::start(const QString& path) {
    reset();

    i32 err;
    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    AVFormatContext* formatContextPtr = formatContext.get();

    err = avformat_open_input(
        &formatContextPtr,
        path.toUtf8().constData(),
        nullptr,
        nullptr
    );

    if (err < 0) {
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return;
    }

    formatContext.reset(formatContextPtr);
    formatContextPtr = formatContext.get();

    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return;
    }

    const AVCodec* codec;
    audioStreamIndex = av_find_best_stream(
        formatContextPtr,
        AVMEDIA_TYPE_AUDIO,
        -1,
        -1,
        &codec,
        0
    );

    err = audioStreamIndex;

    if (err < 0) {
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return;
    }

    const AVStream* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = make_codec_context(codec);

    AVCodecContext* codecContextPtr = codecContext.get();
    avcodec_parameters_to_context(codecContextPtr, audioStream->codecpar);

    err = avcodec_open2(codecContextPtr, codec, nullptr);

    if (err < 0) {
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return;
    }

    const AVChannelLayout channelLayout = codecContext->ch_layout;
    const u8 channelNumber = channelLayout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;

    SwrContext* swrContextPtr = swrContext.get();

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
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return;
    }

    swrContext.reset(swrContextPtr);
    swrContextPtr = swrContext.get();

    swr_init(swrContextPtr);

    packet = make_packet();
    frame = make_frame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;

    format_.setSampleRate(sampleRate);
    format_.setChannelCount(channelNumber);

    filters.resize(channelNumber);

    initFilters();
    prepareBuffer();

    open(QIODevice::ReadOnly);
}

void AudioStreamer::equalizeBuffer(QByteArray& buf) {
    const u8 channels = codecContext->ch_layout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;
    const i32 samplesNumber =
        as<i32>(buf.size() / as<i32>(SAMPLE_SIZE * channels));

    juce::AudioBuffer<f32> juceBuffer(channels, samplesNumber);
    const f32* f32samples = reinterpret_cast<const f32*>(buf.constData());

    for (u8 channel = 0; channel < channels; channel++) {
        f32* floatChannelData = juceBuffer.getWritePointer(channel);

        for (i32 i = 0; i < samplesNumber; i++) {
            floatChannelData[i] = f32samples[(i * channels) + channel];
        }
    }

    for (u8 channel = 0; channel < channels; channel++) {
        f32* writePtr = juceBuffer.getWritePointer(channel);
        juce::dsp::AudioBlock<f32> channelBlock(&writePtr, 1, samplesNumber);
        juce::dsp::ProcessContextReplacing<f32> context(channelBlock);

        for (u8 band = 0; band < bandCount; band++) {
            if (gains_[band] != 0) {
                filters[channel][band].process(context);
            }
        }
    }

    f32* out = reinterpret_cast<f32*>(buf.data());

    for (u8 channel = 0; channel < channels; channel++) {
        const f32* floatChannelData = juceBuffer.getReadPointer(channel);

        for (i32 i = 0; i < samplesNumber; i++) {
            out[(i * channels) + channel] = floatChannelData[i];
        }
    }
}

void AudioStreamer::prepareBuffer() {
    while (true) {
        if (av_read_frame(formatContext.get(), packet.get()) < 0) {
            nextBufferSize = 0;
            break;
        }

        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet.get());
            continue;
        }

        i32 err = avcodec_send_packet(codecContext.get(), packet.get());
        av_packet_unref(packet.get());

        if (err < 0) {
            nextBufferSize = 0;
            break;
        }

        err = avcodec_receive_frame(codecContext.get(), frame.get());

        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF || err < 0) {
            if (err == AVERROR_EOF) {
                nextBufferSize = 0;
            }

            av_frame_unref(frame.get());
            break;
        }

        const i64 timestamp = (frame->pts != AV_NOPTS_VALUE)
                                  ? frame->pts
                                  : frame->best_effort_timestamp;

        playbackSecond = as<u16>(
            as<f64>(timestamp) *
            av_q2d(formatContext->streams[audioStreamIndex]->time_base)
        );

        if (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT) {
            FramePtr newFrame = make_frame();
            newFrame->ch_layout = codecContext->ch_layout;
            newFrame->sample_rate = codecContext->sample_rate;
            newFrame->format = AV_SAMPLE_FMT_FLT;

            swr_convert_frame(swrContext.get(), newFrame.get(), frame.get());

            frame.reset(newFrame.release());
        }

        u16 bufferSize;

        bufferSize = av_samples_get_buffer_size(
            nullptr,
            codecContext->ch_layout.nb_channels,
            frame->nb_samples,
            AV_SAMPLE_FMT_FLT,
            1
        );

        if (bufferSize <= 0) {
            nextBufferSize = 0;
        } else {
            buffer.resize(bufferSize);
            memcpy(buffer.data(), frame->data[0], bufferSize);
            nextBufferSize = bufferSize;
        }

        if (eqEnabled &&
            !ranges::all_of(gains_, [](const i8 gain) { return gain == 0; })) {
            equalizeBuffer(buffer);
        }

        av_frame_unref(frame.get());
        break;
    }
}

auto AudioStreamer::readData(str data, const qi64 /* size */) -> qi64 {
    const u32 bufferSize = buffer.size();
    memcpy(data, buffer.constData(), bufferSize);

    emit progressUpdate(playbackSecond);

    initFilters();
    prepareBuffer();

    if (nextBufferSize == 0) {
        emit streamEnded();
        return -1;
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

    buffer.clear();

    nextBufferSize = 0;
    audioStreamIndex = 0;
    secondsDuration = 0;

    return true;
}

void AudioStreamer::seekSecond(const u16 second) {
    const i64 timestamp = av_rescale(
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

    initFilters();
    prepareBuffer();
}

void AudioStreamer::initFilters() {
    for (auto& channelFilters : filters) {
        if (channelFilters.size() != bandCount) {
            channelFilters.resize(bandCount);
        }
    }

    for (const auto [band, changed] :
         views::enumerate(views::take(changedBands, bandCount))) {
        if (changed) {
            const auto coeffs = IIRCoefficients::makePeakFilter(
                codecContext->sample_rate,
                frequencies[band],
                Q_FACTOR,
                juce::Decibels::decibelsToGain(as<f32>(gains_[band]))
            );

            for (auto& channelFilters : filters) {
                channelFilters[band].coefficients = coeffs;
                channelFilters[band].reset();
            }

            changed = false;
        }
    }
}

void AudioStreamer::setBandCount(const u8 bands) {
    bandCount = bands;

    switch (bands) {
        case THREE_BANDS:
            memcpy(
                frequencies.data(),
                THREE_BAND_FREQUENCIES.data(),
                THREE_BANDS * SAMPLE_SIZE
            );
            break;
        case FIVE_BANDS:
            memcpy(
                frequencies.data(),
                FIVE_BAND_FREQUENCIES.data(),
                FIVE_BANDS * SAMPLE_SIZE
            );
            break;
        case TEN_BANDS:
            memcpy(
                frequencies.data(),
                TEN_BAND_FREQUENCIES.data(),
                TEN_BANDS * SAMPLE_SIZE
            );
            break;
        case EIGHTEEN_BANDS:
            memcpy(
                frequencies.data(),
                EIGHTEEN_BAND_FREQUENCIES.data(),
                EIGHTEEN_BANDS * SAMPLE_SIZE
            );
            break;
        case THIRTY_BANDS:
            memcpy(
                frequencies.data(),
                THIRTY_BAND_FREQUENCIES.data(),
                THIRTY_BANDS * SAMPLE_SIZE
            );
            break;
        default:
            break;
    }

    gains_.fill(0);
    changedBands.fill(false);
}