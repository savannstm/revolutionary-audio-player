#include "audiostreamer.hpp"

#include <libavutil/avutil.h>

constexpr u16 MIN_BUFFER_SIZE = 4096;

AudioStreamer::AudioStreamer(QObject* parent) : QIODevice(parent) {
    format_.setSampleFormat(QAudioFormat::Float);
}

void AudioStreamer::start(const QString& path) {
    reset();

    i32 err;

    AVFormatContext* formatContextPtr = formatContext.get();

    err = avformat_open_input(
        &formatContextPtr,
        path.toUtf8().constData(),
        nullptr,
        nullptr
    );

    if (err < 0) {
        printErr(err);
        return;
    }

    formatContext.reset(formatContextPtr);

    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        printErr(err);
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
        printErr(err);
        return;
    }

    const AVStream* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = make_codec_context(codec);

    AVCodecContext* codecContextPtr = codecContext.get();
    avcodec_parameters_to_context(codecContextPtr, audioStream->codecpar);

    err = avcodec_open2(codecContextPtr, codec, nullptr);

    if (err < 0) {
        printErr(err);
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
        printErr(err);
        return;
    }

    swrContext.reset(swrContextPtr);

    swr_init(swrContextPtr);

    packet = make_packet();
    frame = make_frame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;

    format_.setSampleRate(sampleRate);
    format_.setChannelCount(channelNumber);

    filters.resize(channelNumber);

    formatName = codecContextPtr->codec->name;
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

    initFilters();
    prepareBuffer();

    open(QIODevice::ReadOnly);
}

void AudioStreamer::equalizeBuffer(QByteArray& buf) {
    if (!eqEnabled ||
        ranges::all_of(gains_, [](const i8 gain) { return gain == 0; })) {
        return;
    }

    const u8 channels = codecContext->ch_layout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;
    const i32 samplesNumber = as<i32>(buf.size() / (SAMPLE_SIZE * channels));

    juce::AudioBuffer<f32> juceBuffer(channels, samplesNumber);
    const f32* f32samples = ras<const f32*>(buf.constData());

    for (const u8 channel : range(0, channels)) {
        f32* floatChannelData = juceBuffer.getWritePointer(channel);

        for (const u32 sample : range(0, samplesNumber)) {
            floatChannelData[sample] =
                f32samples[(sample * channels) + channel];
        }
    }

    for (const u8 channel : range(0, channels)) {
        f32* writePtr = juceBuffer.getWritePointer(channel);
        juce::dsp::AudioBlock<f32> channelBlock(&writePtr, 1, samplesNumber);
        juce::dsp::ProcessContextReplacing<f32> context(channelBlock);

        for (const u8 band : range(0, bandCount)) {
            if (gains_[band] != 0) {
                filters[channel][band].process(context);
            }
        }
    }

    f32* out = ras<f32*>(buf.data());

    for (const u8 channel : range(0, channels)) {
        const f32* floatChannelData = juceBuffer.getReadPointer(channel);

        for (const u32 sample : range(0, samplesNumber)) {
            out[(sample * channels) + channel] = floatChannelData[sample];
        }
    }
}

auto AudioStreamer::processFrame() -> bool {
    const f64 timestamp = as<f64>(
        (frame->pts != AV_NOPTS_VALUE) ? frame->pts
                                       : frame->best_effort_timestamp
    );

    playbackSecond = as<u16>(
        timestamp * av_q2d(formatContext->streams[audioStreamIndex]->time_base)
    );

    if (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT) {
        AVFrame* newFrame = av_frame_alloc();
        newFrame->ch_layout = codecContext->ch_layout;
        newFrame->sample_rate = codecContext->sample_rate;
        newFrame->format = AV_SAMPLE_FMT_FLT;

        swr_convert_frame(swrContext.get(), newFrame, frame.get());
        frame.reset(newFrame);
    }

    const u32 bufferSize =
        frame->nb_samples * SAMPLE_SIZE * frame->ch_layout.nb_channels;

    buffer.append(ras<cstr>(frame->data[0]), bufferSize);

    nextBufferSize = buffer.size();
    return nextBufferSize >= MIN_BUFFER_SIZE;
}

// TODO: For some reason, skips a little of data at the start
// Only handles pcm_i*le and pcm_f32
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

    const u32 sampleCount = bytesRead / inputSampleSize;

    QByteArray tempBuffer;
    tempBuffer.resize(as<u32>(sampleCount * SAMPLE_SIZE));

    f32* floatSamples = ras<f32*>(tempBuffer.data());

    switch (inputSampleSize) {
        case sizeof(i8): {
            const i8* samples = ras<const i8*>(buffer.constData());

            for (const u32 idx : range(0, sampleCount)) {
                floatSamples[idx] = as<f32>(samples[idx]) / (INT8_MAX + 1.0F);
            }
            break;
        }
        case sizeof(i16): {
            const i16* samples = ras<const i16*>(buffer.constData());

            for (const u32 idx : range(0, sampleCount)) {
                floatSamples[idx] = as<f32>(samples[idx]) / (INT16_MAX + 1.0F);
            }
            break;
        }
        case sizeof(i8) + sizeof(i16): {
            constexpr u8 I8_BIT = sizeof(i8) * CHAR_BIT;
            constexpr u8 I16_BIT = sizeof(i16) * CHAR_BIT;
            constexpr u32 INT24_SIGN_MASK = as<u32>(INT32_MAX) + 1;
            constexpr i32 INT24_MAX = 16777215;

            const u8* samples = ras<const u8*>(buffer.constData());

            for (const u32 idx : range(0, sampleCount)) {
                const u32 offset = idx * inputSampleSize;
                i32 value = (samples[offset]) |
                            (samples[offset + 1] << I8_BIT) |
                            (samples[offset + 2] << I16_BIT);

                if ((value & INT24_SIGN_MASK) != 0) {
                    value |= ~INT24_MAX;
                }

                floatSamples[idx] =
                    as<f32>(value) / (as<f32>(1 << ((I8_BIT + I16_BIT) - 1)));
            }
            break;
        }
        case sizeof(i32): {
            if (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT) {
                const i32* samples = ras<const i32*>(buffer.constData());

                for (const u32 idx : range(0, sampleCount)) {
                    floatSamples[idx] =
                        as<f32>(samples[idx]) / as<f32>(INT32_MAX);
                }
            } else {
                memcpy(
                    tempBuffer.data(),
                    buffer.constData(),
                    tempBuffer.size()
                );
            }
            break;
        }
        default:
            break;
    }

    buffer = std::move(tempBuffer);
    nextBufferSize = buffer.size();
}

void AudioStreamer::prepareBuffer() {
    nextBufferSize = 0;
    buffer.clear();

    if (formatName.starts_with("pcm")) {
        decodeRaw();
        equalizeBuffer(buffer);
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
                equalizeBuffer(buffer);
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

    initFilters();
    prepareBuffer();

    if (nextBufferSize == 0) {
        emit streamEnded();
        // Fuck it, just don't return anything
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

    if (string_view(codecContext->codec->name).starts_with("pcm") &&
        (formatContext->pb != nullptr)) {
        totalBytesRead = avio_tell(formatContext->pb);
    }

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
                frequencies_[band],
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
}