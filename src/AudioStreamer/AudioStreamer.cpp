#include "AudioStreamer.hpp"

#include "Constants.hpp"
#include "Enums.hpp"
#include "Utils.hpp"

#include <QDebug>

struct Filter {
    const cstr filter;
    const cstr name;
    const cstr args;
    AVFilterContext** const context;
};

AudioStreamer::AudioStreamer(const shared_ptr<Settings>& settings) :
    eqSettings(settings->equalizer),
    visSettings(settings->spectrumVisualizer) {
#ifdef DEBUG_BUILD
    av_log_set_level(AV_LOG_VERBOSE);
#elifdef RELEASE_BUILD
    av_log_set_level(AV_LOG_QUIET);
#endif
}

auto AudioStreamer::start(const string& path, const i32 startSecond)
    -> result<std::monostate, QString> {
    reset();

    AVFormatContext* fCtxPtr = nullptr;

    i32 err = avformat_open_input(&fCtxPtr, path.c_str(), nullptr, nullptr);

    if (checkError(err, true, false)) {
        return Err(FFMPEG_ERROR(err));
    }

    formatContext.reset(fCtxPtr);
    const i64 headerLength = avio_tell(formatContext->pb);

    err = avformat_find_stream_info(fCtxPtr, nullptr);

    if (checkError(err, true, false)) {
        return Err(FFMPEG_ERROR(err));
    }

    const AVCodec* codec = nullptr;
    err = av_find_best_stream(fCtxPtr, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

    if (checkError(err, true, false)) {
        return Err(FFMPEG_ERROR(err));
    }

    audioStreamIndex = u8(err);
    audioStream = fCtxPtr->streams[audioStreamIndex];

    codecContext = createCodecContext(codec);
    avcodec_parameters_to_context(codecContext.get(), audioStream->codecpar);
    err = avcodec_open2(codecContext.get(), codec, nullptr);

    if (checkError(err, true, false)) {
        return Err(FFMPEG_ERROR(err));
    }

    const string_view formatName = codecContext->codec->name;
    rawPCM = formatName.starts_with("pcm");

    const u8 sampleSize_ = av_get_bytes_per_sample(codecContext->sample_fmt);
    switch (sampleSize_) {
        case 2:
            sampleSize = SampleSize::S16;
            break;
        case 3:
            sampleSize = SampleSize::S24;
            break;
        case 4:
            sampleSize = SampleSize::S32;
            break;
        default:
            return Err(
                QObject::tr("Unsupported sample size: %1").arg(sampleSize_)
            );
    }

    if (codecContext->ch_layout.nb_channels > UINT8_MAX) {
        return Err(QObject::tr("More than 255 channels are unsupported."));
    }

    if (rawPCM) {
        // Ensure that we are at the very start of the data,
        // when reading raw PCM
        avio_seek(formatContext->pb, headerLength, SEEK_SET);

        if (formatName.ends_with("24le")) {
            sampleSize = SampleSize::S24;
        }

        const i64 streamDuration = audioStream->duration;
        const f64 durationSeconds =
            f64(streamDuration) * av_q2d(audioStream->time_base);
        rawEndPos = i64(durationSeconds * f64(codecContext->bit_rate));
    } else {
        // We need that only in encoded formats
        packet = createPacket();
        frame = createFrame();
    }

    const u16 frameSize = codecContext->ch_layout.nb_channels * F32_SIZE;
    copySize_ =
        u16(floorf(f32(MIN_BUFFER_SIZE) / f32(frameSize)) * f32(frameSize));

    const AVCodecParameters* const codecpar = audioStream->codecpar;
    const AVCodecID codecID = codecpar->codec_id;

    switch (codecID) {
        case AV_CODEC_ID_FLAC:
            // FLAC stores frame size in STREAMINFO block. Its layout in
            // described in
            // https://www.rfc-editor.org/rfc/rfc9639.html#name-streaminfo
            bufferThreshold =
                std::byteswap(ras<u16*>(codecpar->extradata)[1]) * frameSize;
            break;
        case AV_CODEC_ID_MP3:
        case AV_CODEC_ID_AAC:
            bufferThreshold = codecContext->frame_size * frameSize;
            break;
        case AV_CODEC_ID_AC3:
        case AV_CODEC_ID_EAC3:
            bufferThreshold = MAX_AC3_FRAME_SAMPLES * frameSize;
            break;
        case AV_CODEC_ID_ALAC: {
            // ALAC stores frame size in alac atom. Its layout is described in
            // libavcodec/alac.c
            bufferThreshold =
                std::byteswap(ras<u32*>(codecpar->extradata)[3]) * frameSize;
            break;
        }
        case AV_CODEC_ID_OPUS:
            bufferThreshold = MAX_OPUS_FRAME_SAMPLES * frameSize;
            break;
        case AV_CODEC_ID_VORBIS:
            bufferThreshold = MAX_VORBIS_FRAME_SAMPLES * frameSize;
            break;
        default:
            bufferThreshold = RAW_BUFFER_THRESHOLD;
            break;
    }

    bufferThreshold = max<u32>(bufferThreshold, MIN_BUFFER_THRESHOLD);
    bufSize = std::bit_ceil(max<u32>(bufferThreshold * 2, MIN_BUF_SIZE));
    bufSizeMask = bufSize - 1;
    buf = new u8[bufSize];
    memset(buf, 0, bufSize);

    initializeFilters(true);

    if (startSecond != -1) {
        seekSecond(startSecond);
    } else {
        readData();
    }

    return std::monostate();
}

auto AudioStreamer::reset() -> bool {
    formatContext.reset();
    codecContext.reset();
    packet.reset();
    frame.reset();
    filterGraph.reset();

    delete[] buf;
    buf = nullptr;

    flush();
    streamFinished_ = false;

    return true;
}

auto AudioStreamer::consumeBlock() -> optional<Block> {
    if (streamFinished_ && headOffset >= tailOffset) {
        return nullopt;
    }

    const u32 bytesToRead =
        min<u32>(copySize_, dataSize.load(std::memory_order_acquire));
    const u32 firstChunk = min(bytesToRead, bufSize - headOffset);

    const Block block{
        .firstPart = span(buf + headOffset, firstChunk),
        .secondPart = bytesToRead > firstChunk
                          ? span(buf, bytesToRead - firstChunk)
                          : span<u8>{},
        .second = samplePos.load(std::memory_order_acquire) /
                  codecContext->sample_rate,
    };

    samplePos.fetch_add(
        bytesToRead / (codecContext->ch_layout.nb_channels * F32_SIZE),
        std::memory_order_acq_rel
    );
    headOffset = (headOffset + bytesToRead) & bufSizeMask;
    dataSize.fetch_sub(bytesToRead, std::memory_order_acq_rel);

    return block;
}

void AudioStreamer::writeToRingBuf(
    const u32 offset,
    const void* __restrict const src,
    const u32 size
) {
    const u32 firstChunk = (offset + size > bufSize) ? bufSize - offset : size;
    memcpy(buf + offset, src, firstChunk);

    if (size > firstChunk) {
        memcpy(buf, as<const u8*>(src) + firstChunk, size - firstChunk);
    }
}

void AudioStreamer::readFromRingBuf(
    const u32 offset,
    void* __restrict const dst,
    const u32 size
) {
    const u32 firstChunk = (offset + size > bufSize) ? bufSize - offset : size;
    memcpy(dst, buf + offset, firstChunk);

    if (size > firstChunk) {
        memcpy(as<u8*>(dst) + firstChunk, buf, size - firstChunk);
    }
}

void AudioStreamer::readData() {
    if (streamFinished_ ||
        dataSize.load(std::memory_order_acquire) > bufferThreshold) {
        return;
    }

    if (rawPCM) {
        readRaw();
        return;
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
                av_frame_unref(frame.get());
                break;
            }

            readFrame();
            av_frame_unref(frame.get());

            if (dataSize.load(std::memory_order_acquire) < bufferThreshold) {
                break;
            }

            return;
        }
    }

    streamFinished_ = true;
}

void AudioStreamer::readRaw() {
    const i64 currentPos = avio_tell(formatContext->pb);
    constexpr u32 MAX_RAW_READ_SIZE = UINT16_MAX + 1;
    const u8 sampleBytes = sampleSize;
    const u32 bytesPerFrame = codecContext->ch_layout.nb_channels * sampleBytes;

    i64 remainingBytes = rawEndPos - currentPos;
    if (remainingBytes <= 0) {
        streamFinished_ = true;
        return;
    }

    u32 bytesToRead = min<u32>(MAX_RAW_READ_SIZE, u32(remainingBytes));
    bytesToRead -= bytesToRead % bytesPerFrame;

    if (bytesToRead == 0) {
        streamFinished_ = true;
        return;
    }

    array<u8, MAX_RAW_READ_SIZE> rawBuffer;
    const i32 read =
        avio_read(formatContext->pb, rawBuffer.data(), i32(bytesToRead));

    if (read <= 0) {
        streamFinished_ = true;
        return;
    }

    u32 inputBytes = u32(read);
    inputBytes -= inputBytes % bytesPerFrame;

    if (inputBytes == 0) {
        streamFinished_ = true;
        return;
    }

    u32 outputBytes;

    if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLT) {
        outputBytes = inputBytes;
        writeToRingBuf(tailOffset, rawBuffer.data(), outputBytes);
    } else {
        switch (sampleSize) {
            case SampleSize::S16: {
                const i16* __restrict const input = ras<i16*>(rawBuffer.data());
                const u32 sampleCount = inputBytes / 2;
                outputBytes = sampleCount * F32_SIZE;

                const u32 firstChunk =
                    min(outputBytes, bufSize - tailOffset) / F32_SIZE;
                const u32 secondChunk = sampleCount - firstChunk;

                f32* __restrict out = ras<f32*>(buf + tailOffset);

                for (const u32 idx : range<u32>(0, firstChunk)) {
                    out[idx] = f32(input[idx]) / I16_DIVISOR;
                }

                out = ras<f32*>(buf);

                for (const u32 idx : range<u32>(0, secondChunk)) {
                    out[idx] = f32(input[firstChunk + idx]) / I16_DIVISOR;
                }
                break;
            }
            case SampleSize::S24: {
                const u8* __restrict input = rawBuffer.data();
                const u32 sampleCount = inputBytes / 3;
                outputBytes = sampleCount * F32_SIZE;

                const u32 firstChunk =
                    min(outputBytes, bufSize - tailOffset) / F32_SIZE;
                const u32 secondChunk = sampleCount - firstChunk;

                f32* __restrict out = ras<f32*>(buf + tailOffset);

                for (const u32 idx : range<u32>(0, firstChunk)) {
                    i32 value = input[0] | (input[1] << CHAR_BIT) |
                                (input[2] << (I16_SIZE * CHAR_BIT));

                    if ((value & (INT24_MAX + 1)) != 0) {
                        value |= ~UINT24_MAX;
                    }

                    out[idx] = f32(value) / I24_DIVISOR;
                    input += I24_SIZE;
                }

                out = ras<f32*>(buf);

                for (const u32 idx : range<u32>(0, secondChunk)) {
                    i32 value = input[0] | (input[1] << CHAR_BIT) |
                                (input[2] << (I16_SIZE * CHAR_BIT));

                    if ((value & (INT24_MAX + 1)) != 0) {
                        value |= ~UINT24_MAX;
                    }

                    out[idx] = f32(value) / I24_DIVISOR;
                    input += I24_SIZE;
                }
                break;
            }
            case SampleSize::S32: {
                const i32* __restrict const input = ras<i32*>(rawBuffer.data());
                const u32 sampleCount = inputBytes / 4;
                outputBytes = sampleCount * F32_SIZE;

                const u32 firstChunk =
                    min(outputBytes, bufSize - tailOffset) / F32_SIZE;
                const u32 secondChunk = sampleCount - firstChunk;

                f32* __restrict out = ras<f32*>(buf + tailOffset);

                for (const u32 idx : range<u32>(0, firstChunk)) {
                    out[idx] = f32(input[idx]) / I32_DIVISOR;
                }

                out = ras<f32*>(buf);

                for (const u32 idx : range<u32>(0, secondChunk)) {
                    out[idx] = f32(input[firstChunk + idx]) / I32_DIVISOR;
                }
                break;
            }
            default:
                std::unreachable();
        }
    }

    equalizeData(tailOffset, outputBytes);

    if (seekPerformed) {
        u8 multiplier;

        switch (sampleSize) {
            case SampleSize::S16:
                multiplier = 2;
                break;
            case SampleSize::S24:
                multiplier = 3;
                break;
            case SampleSize::S32:
                multiplier = 4;
                break;
        }

        samplePos.store(
            u32(currentPos + inputBytes) /
                u32(codecContext->ch_layout.nb_channels * multiplier),
            std::memory_order_release
        );
        seekPerformed = false;
    }
    tailOffset = (tailOffset + outputBytes) & bufSizeMask;
    dataSize.fetch_add(outputBytes, std::memory_order_acq_rel);
}

void AudioStreamer::readPlanar1Ch(
    f32* __restrict dst,
    const u8* const __restrict* const __restrict src,
    const u32 firstChunk,
    const u32 secondChunk
) {
    switch (sampleSize) {
        case SampleSize::S16: {
            const i16* __restrict const input = ras<const i16*>(src[0]);

            for (const u32 sample : range<u32>(0, firstChunk)) {
                dst[sample] = f32(input[sample]) / I16_DIVISOR;
            }

            dst = ras<f32*>(buf);

            for (const u32 sample : range<u32>(0, secondChunk)) {
                dst[sample] = f32(input[firstChunk + sample]) / I16_DIVISOR;
            }
            break;
        }
        case SampleSize::S32: {
            if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                const f32* __restrict const input = ras<const f32*>(src[0]);

                for (const u32 sample : range<u32>(0, firstChunk)) {
                    dst[sample] = input[sample];
                }

                dst = ras<f32*>(buf);

                for (const u32 sample : range<u32>(0, secondChunk)) {
                    dst[sample] = input[firstChunk + sample];
                }
            } else {
                const i32* __restrict const input = ras<const i32*>(src[0]);

                for (const u32 sample : range<u32>(0, firstChunk)) {
                    dst[sample] = f32(input[sample]) / I32_DIVISOR;
                }

                dst = ras<f32*>(buf);

                for (const u32 sample : range<u32>(0, secondChunk)) {
                    dst[sample] = f32(input[firstChunk + sample]) / I32_DIVISOR;
                }
            }

            break;
        }
        default:
            break;
    }
}

// This optimizes pretty good. However, manual intrinsic optimization for <=4
// and maybe <=8 channels is possible.
void AudioStreamer::readPlanarMultichannel(
    f32* __restrict dst,
    const u8* const __restrict* const __restrict src,
    const u32 firstChunk,  // NOLINT
    const u32 secondChunk
) {
    const u32 completeFrames = firstChunk / codecContext->ch_layout.nb_channels;
    const u32 partialChannels =
        firstChunk % codecContext->ch_layout.nb_channels;

    switch (sampleSize) {
        case SampleSize::S16:
            for (const u8 channel :
                 range<u8>(0, codecContext->ch_layout.nb_channels)) {
                const i16* __restrict const input =
                    ras<const i16*>(src[channel]);
                const u32 numSamples =
                    completeFrames + (channel < partialChannels ? 1 : 0);

                for (const u32 sample : range<u32>(0, numSamples)) {
                    const u32 index =
                        (sample * codecContext->ch_layout.nb_channels) +
                        channel;
                    dst[index] = f32(input[sample]) / I16_DIVISOR;
                }
            }
            break;

        case SampleSize::S32:
            if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                for (const u8 channel :
                     range<u8>(0, codecContext->ch_layout.nb_channels)) {
                    const f32* __restrict const input =
                        ras<const f32*>(src[channel]);
                    const u32 numSamples =
                        completeFrames + (channel < partialChannels ? 1 : 0);

                    for (const u32 sample : range<u32>(0, numSamples)) {
                        const u32 index =
                            (sample * codecContext->ch_layout.nb_channels) +
                            channel;
                        dst[index] = input[sample];
                    }
                }
            } else {
                for (const u8 channel :
                     range<u8>(0, codecContext->ch_layout.nb_channels)) {
                    const i32* __restrict const input =
                        ras<const i32*>(src[channel]);
                    const u32 numSamples =
                        completeFrames + (channel < partialChannels ? 1 : 0);

                    for (const u32 sample : range<u32>(0, numSamples)) {
                        const u32 index =
                            (sample * codecContext->ch_layout.nb_channels) +
                            channel;
                        dst[index] = f32(input[sample]) / I32_DIVISOR;
                    }
                }
            }
            break;

        default:
            std::unreachable();
    }

    u32 breakSample = completeFrames;
    u8 breakChannel = partialChannels;

    if (breakChannel == codecContext->ch_layout.nb_channels) {
        breakSample++;
        breakChannel = 0;
    }

    dst = ras<f32*>(buf);

    switch (sampleSize) {
        case SampleSize::S16:
            for (const u8 channel :
                 range<u8>(0, codecContext->ch_layout.nb_channels)) {
                const i16* __restrict input = ras<const i16*>(src[channel]);
                const u32 startSample =
                    (channel < breakChannel) ? breakSample + 1 : breakSample;

                for (const u32 sample :
                     range<u32>(startSample, frame->nb_samples)) {
                    const u32 index =
                        (sample * codecContext->ch_layout.nb_channels) +
                        channel - firstChunk;
                    dst[index] = f32(input[sample]) / I16_DIVISOR;
                }
            }
            break;

        case SampleSize::S32:
            if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLTP) {
                for (const u8 channel :
                     range<u8>(0, codecContext->ch_layout.nb_channels)) {
                    const f32* __restrict input = ras<const f32*>(src[channel]);
                    const u32 startSample = (channel < breakChannel)
                                                ? breakSample + 1
                                                : breakSample;

                    for (const u32 sample :
                         range<u32>(startSample, frame->nb_samples)) {
                        const u32 index =
                            (sample * codecContext->ch_layout.nb_channels) +
                            channel - firstChunk;
                        dst[index] = input[sample];
                    }
                }
            } else {
                for (const u8 channel :
                     range<u8>(0, codecContext->ch_layout.nb_channels)) {
                    const i32* __restrict input = ras<const i32*>(src[channel]);
                    const u32 startSample = (channel < breakChannel)
                                                ? breakSample + 1
                                                : breakSample;

                    for (const u32 sample :
                         range<u32>(startSample, frame->nb_samples)) {
                        const u32 index =
                            (sample * codecContext->ch_layout.nb_channels) +
                            channel - firstChunk;
                        dst[index] = f32(input[sample]) / I32_DIVISOR;
                    }
                }
            }
            break;

        default:
            std::unreachable();
    }
}

void AudioStreamer::readFrame() {
    if (seekPerformed) {
        const i64 timestamp = (frame->pts != AV_NOPTS_VALUE)
                                  ? frame->pts
                                  : frame->best_effort_timestamp;
        samplePos.store(
            av_rescale_q(
                timestamp,
                audioStream->time_base,
                { 1, frame->sample_rate }
            ),
            std::memory_order_release
        );
        seekPerformed = false;
    }

    const u8* __restrict const* __restrict src =
        (frame->extended_data != nullptr) ? frame->extended_data : frame->data;
    const u32 sampleCount =
        frame->nb_samples * codecContext->ch_layout.nb_channels;
    const u32 decodedSize = sampleCount * F32_SIZE;

    f32* __restrict dst = ras<f32*>(buf + tailOffset);

    const u32 firstChunk = min(decodedSize, bufSize - tailOffset) / F32_SIZE;
    const u32 secondChunk = sampleCount - firstChunk;

    if (bool(av_sample_fmt_is_planar(codecContext->sample_fmt))) {
        if (codecContext->ch_layout.nb_channels == 1) {
            readPlanar1Ch(dst, src, firstChunk, secondChunk);
            goto end;
        }

        // TODO: Optimize 2 channels with hand-written AVX?

        readPlanarMultichannel(dst, src, firstChunk, secondChunk);
    } else {
        const u8* __restrict const input = src[0];

        if (codecContext->sample_fmt == AV_SAMPLE_FMT_FLT) {
            writeToRingBuf(tailOffset, input, decodedSize);
            goto end;
        }

        switch (sampleSize) {
            case SampleSize::S16:
                for (const u32 sample : range<u32>(0, firstChunk)) {
                    dst[sample] =
                        f32(ras<const i16*>(input)[sample]) / I16_DIVISOR;
                }

                dst = ras<f32*>(buf);

                for (const u32 sample : range<u32>(0, secondChunk)) {
                    dst[sample] =
                        f32(ras<const i16*>(input)[firstChunk + sample]) /
                        I16_DIVISOR;
                }
                break;
            case SampleSize::S32:
                for (const u32 sample : range<u32>(0, firstChunk)) {
                    dst[sample] =
                        f32(ras<const i32*>(input)[sample]) / I32_DIVISOR;
                }

                dst = ras<f32*>(buf);

                for (const u32 sample : range<u32>(0, secondChunk)) {
                    dst[sample] =
                        f32(ras<const i32*>(input)[firstChunk + sample]) /
                        I32_DIVISOR;
                }
                break;
            default:
                std::unreachable();
        }
    }

end:
    equalizeData(tailOffset, decodedSize);

    tailOffset = (tailOffset + decodedSize) & bufSizeMask;
    dataSize.fetch_add(decodedSize, std::memory_order_acq_rel);
}

void AudioStreamer::equalizeData(const u32 offset, const u32 size) {
    initializeFilters();

    if (!eqSettings.enabled || filterGraph == nullptr ||
        ranges::all_of(
            span<i8>(eqSettings.gains.data(), usize(eqSettings.bandCount)),
            [](const i8 gain) -> bool { return gain == 0; }
        )) {
        return;
    }

    AVFrame* unfilteredFrame = av_frame_alloc();
    unfilteredFrame->nb_samples =
        i32(size / (F32_SIZE * codecContext->ch_layout.nb_channels));
    unfilteredFrame->format = AV_SAMPLE_FMT_FLT;
    unfilteredFrame->ch_layout = codecContext->ch_layout;
    unfilteredFrame->sample_rate = codecContext->sample_rate;
    av_frame_get_buffer(unfilteredFrame, 0);

    readFromRingBuf(offset, unfilteredFrame->data[0], size);

    i32 err = av_buffersrc_add_frame(abufferContext, unfilteredFrame);
    if (checkError(err, false, false)) {
        av_frame_free(&unfilteredFrame);
        return;
    }

    AVFrame* filteredFrame = av_frame_alloc();
    err = av_buffersink_get_frame(abuffersinkContext, filteredFrame);

    if (checkError(err, false, false)) {
        av_frame_free(&filteredFrame);
        av_frame_free(&unfilteredFrame);
        return;
    }

    const u32 filteredSize = u32(filteredFrame->nb_samples) *
                             codecContext->ch_layout.nb_channels * F32_SIZE;
    const u32 filteredBytesToCopy = min(size, filteredSize);

    if (filteredBytesToCopy > 0) {
        writeToRingBuf(offset, filteredFrame->data[0], filteredBytesToCopy);
    }

    // If the filter chain returned fewer samples (e.g. internal delay), keep
    // the remaining original samples to avoid reading beyond filtered frame.
    if (filteredBytesToCopy < size) {
        writeToRingBuf(
            (offset + filteredBytesToCopy) & bufSizeMask,
            unfilteredFrame->data[0] + filteredBytesToCopy,
            size - filteredBytesToCopy
        );
    }

    av_frame_free(&filteredFrame);
    av_frame_free(&unfilteredFrame);
}

void AudioStreamer::seekSecond(const i32 second) {
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

    seekPerformed = true;

    flush();
    readData();
}

void AudioStreamer::initializeFilters(const bool force) {
    // We don't ever need to rebuild the filters, equalizer's gains are
    // adjusted through the command, if they need to be changed
    if (!force && (!eqSettings.enabled || filterGraph != nullptr)) {
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
        qWarning() << "Could not allocate filter graph."_L1;
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
    // Hamming window function is suggested by ChatGPT as the best for
    // general EQ
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
    // `abuffer` always must be first, and `aformat` with `abuffersink`
    // last.
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
    for (const auto filter : filters) {
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
    for (const u8 idx : range<u8>(0, filters.size() - 1)) {
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

auto AudioStreamer::checkError(
    const i32 err,
    const bool resetStreamer,
    const bool resetFilters
) -> bool {
    if (err < 0) {
        qWarning() << FFMPEG_ERROR(err);

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
