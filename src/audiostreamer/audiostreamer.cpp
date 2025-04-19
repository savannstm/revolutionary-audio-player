#include "audiostreamer.hpp"

#include "aliases.hpp"
#include "constants.hpp"

#include <libavutil/avutil.h>

#include <QDebug>

AudioStreamer::AudioStreamer(QObject* parent) : QIODevice(parent) {
    audioFormat.setSampleFormat(QAudioFormat::Float);
}

auto AudioStreamer::start(const path& path) -> bool {
    reset();

    i32 err;
    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    AVFormatContext* formatContextPtr = formatContext.get();

    err = avformat_open_input(
        &formatContextPtr,
        path.string().c_str(),
        nullptr,
        nullptr
    );

    if (err < 0) {
        qWarning() << format(
            "Failed to open input file: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
    }

    formatContext.reset(formatContextPtr);
    formatContextPtr = formatContext.get();

    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        qWarning() << format(
            "Failed to find stream info: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
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
        return false;
    }

    const AVStream* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = make_codec_context(codec);

    AVCodecContext* codecContextPtr = codecContext.get();
    avcodec_parameters_to_context(codecContextPtr, audioStream->codecpar);

    err = avcodec_open2(codecContextPtr, codec, nullptr);

    if (err < 0) {
        qWarning(
        ) << av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err);
        return false;
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
        return false;
    }

    swrContext.reset(swrContextPtr);
    swrContextPtr = swrContext.get();

    swr_init(swrContextPtr);

    packet = make_packet();
    frame = make_frame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;

    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelCount(channelNumber);

    initFilters();
    prepareBuffer();

    return open(QIODevice::ReadOnly);
}

inline void AudioStreamer::equalizeBuffer(QByteArray& buf) {
    const u8 channels = codecContext->ch_layout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;
    const i32 samplesNumber =
        static_cast<i32>(buf.size() / static_cast<i32>(SAMPLE_SIZE * channels));

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

        for (u8 band = 0; band < EQ_BANDS_N; band++) {
            filters[channel][band].process(context);
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

inline void AudioStreamer::prepareBuffer() {
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

        playbackSecond = static_cast<u16>(
            static_cast<f64>(timestamp) *
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
            !ranges::all_of(dbGains, [](const i8 gain) { return gain == 0; })) {
            equalizeBuffer(buffer);
        }

        av_frame_unref(frame.get());
        break;
    }
}

auto AudioStreamer::readData(str data, const qi64 maxSize) -> qi64 {
    const u32 bufferSize = buffer.size();

    memcpy(data, buffer.constData(), bufferSize);

    emit progressUpdate(second());

    initFilters();
    prepareBuffer();

    if (nextBufferSize == 0) {
        emit endOfFile();
        return -1;
    }

    return bufferSize;
}

auto AudioStreamer::bytesAvailable() const -> qi64 {
    return nextBufferSize;
}

auto AudioStreamer::atEnd() const -> bool {
    return nextBufferSize == 0;
}

// Not for write
auto AudioStreamer::writeData(cstr /* data */, qi64 /* size */) -> qi64 {
    return -1;
}

auto AudioStreamer::duration() const -> u16 {
    return secondsDuration;
}

auto AudioStreamer::getFormat() const -> QAudioFormat {
    return audioFormat;
}

inline auto AudioStreamer::second() const -> u16 {
    return playbackSecond;
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

auto AudioStreamer::seekSecond(const u16 second) -> bool {
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

    return true;
}

auto AudioStreamer::isEqEnabled() const -> bool {
    return eqEnabled;
}

void AudioStreamer::setEqEnabled(const bool enabled) {
    eqEnabled = enabled;
}

void AudioStreamer::setGain(const i8 gain, const u8 band) {
    qDebug() << "setting gain";
    dbGains[band] = gain;
    changedBands[band] = true;
}

auto AudioStreamer::getGain(const u8 band) -> i8 {
    return dbGains[band];
}

inline void AudioStreamer::initFilters() {
    for (auto [band, changed] : views::enumerate(changedBands)) {
        if (changed) {
            auto coeffs = juce::dsp::IIR::Coefficients<f32>::makePeakFilter(
                codecContext->sample_rate,
                FREQUENCIES[band],
                QFactor,
                juce::Decibels::decibelsToGain(static_cast<f32>(dbGains[band]))
            );

            for (auto& filter :
                 views::take(filters, codecContext->ch_layout.nb_channels)) {
                filter[band].coefficients = coeffs;
                filter[band].reset();
            }

            changed = false;
        }
    }
}