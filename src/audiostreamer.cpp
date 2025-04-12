#include "audiostreamer.hpp"

#include "aliases.hpp"
#include "constants.hpp"
#include "mainwindow.hpp"

#include "libavformat/avformat.h"

#include <QDebug>
#include <memory>
#include <qaudioformat.h>

auto AudioStreamer::start(const path& path) -> bool {
    reset();

    i32 err;
    array<char, AV_ERROR_MAX_STRING_SIZE> errBuf;

    auto* formatContextPtr = formatContext.get();

    err = avformat_open_input(
        &formatContextPtr,
        path.string().c_str(),
        nullptr,
        nullptr
    );

    if (err < 0) {
        throw panic(
            std::format(
                "Failed to open input file: {}",
                av_make_error_string(
                    errBuf.data(),
                    AV_ERROR_MAX_STRING_SIZE,
                    err
                )
            )
        );
    }

    formatContext.reset(formatContextPtr);

    err = avformat_find_stream_info(formatContext.get(), nullptr);

    if (err < 0) {
        throw panic(
            std::format(
                "Failed to find stream info: {}",
                av_make_error_string(
                    errBuf.data(),
                    AV_ERROR_MAX_STRING_SIZE,
                    err
                )
            )
        );
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
        throw panic(
            std::format(
                "No audio stream found: {}",
                av_make_error_string(
                    errBuf.data(),
                    AV_ERROR_MAX_STRING_SIZE,
                    err
                )
            )
        );
    }

    const auto* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = make_codec_context(codec);
    codecContext->thread_count = 0;
    codecContext->thread_type = FF_THREAD_FRAME;

    AVCodecContext* codecContextPtr = codecContext.get();
    avcodec_parameters_to_context(codecContextPtr, audioStream->codecpar);

    err = avcodec_open2(codecContextPtr, codec, nullptr);

    if (err < 0) {
        throw panic(
            std::format(
                "Failed to open codec: {}",
                av_make_error_string(
                    errBuf.data(),
                    AV_ERROR_MAX_STRING_SIZE,
                    err
                )
            )
        );
    }

    channelLayout = codecContext->ch_layout;
    channels = channelLayout.nb_channels;
    sampleRate = codecContext->sample_rate;
    frames = codecContext->frame_num;

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, channels);

    auto* swrContextPtr = swrContext.get();

    err = swr_alloc_set_opts2(
        &swrContextPtr,
        &outLayout,
        AV_SAMPLE_FMT_S16,
        sampleRate,
        &channelLayout,
        codecContext->sample_fmt,
        sampleRate,
        0,
        nullptr
    );

    if (err < 0) {
        throw panic(
            std::format(
                "Error allocating SwrContext: {}",
                av_make_error_string(
                    errBuf.data(),
                    AV_ERROR_MAX_STRING_SIZE,
                    err
                )
            )
        );
    }

    swrContext.reset(swrContextPtr);
    swr_init(swrContext.get());

    packet = make_packet();
    frame = make_frame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;

    availableBytes =
        static_cast<i32>(sampleRate * channels * SAMPLE_SIZE * secondsDuration);

    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelCount(channels);
    av_channel_layout_uninit(&outLayout);

    return open(QIODevice::ReadOnly);
}

auto AudioStreamer::readData(char* data, i64 maxSize) -> i64 {
    i64 bytes_read = 0;

    while (bytes_read < maxSize) {
        if (position < buffer.size()) {
            const u32 chunkSize =
                std::min(maxSize - bytes_read, buffer.size() - position);

            memcpy(data + bytes_read, buffer.constData() + position, chunkSize);
            position += chunkSize;
            bytes_read += chunkSize;

            if (position >= buffer.size()) {
                buffer.clear();
                position = 0;

                auto* window = static_cast<MainWindow*>(parent());
                QMetaObject::invokeMethod(window, &MainWindow::updateProgress);
            }

            continue;
        }

        if (av_read_frame(formatContext.get(), packet.get()) < 0) {
            break;
        }

        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet.get());
            continue;
        }

        i32 err = avcodec_send_packet(codecContext.get(), packet.get());
        av_packet_unref(packet.get());

        if (err < 0) {
            break;
        }

        while (err >= 0) {
            err = avcodec_receive_frame(codecContext.get(), frame.get());

            if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
                av_frame_unref(frame.get());
                break;
            }

            if (err < 0) {
                av_frame_unref(frame.get());
                return -1;
            }

            const u16 bufferSize = frame->nb_samples * channels * SAMPLE_SIZE;

            QByteArray newBuffer = QByteArray();
            newBuffer.resize(bufferSize);
            numBytesRead += bufferSize;

            if (codecContext->sample_fmt != AV_SAMPLE_FMT_S16) {
                FramePtr newFrame = make_frame();
                newFrame->ch_layout = channelLayout;
                newFrame->sample_rate = sampleRate;
                newFrame->format = AV_SAMPLE_FMT_S16;

                swr_convert_frame(
                    swrContext.get(),
                    newFrame.get(),
                    frame.get()
                );

                memcpy(newBuffer.data(), newFrame->data[0], bufferSize);

                av_frame_unref(newFrame.get());
            } else {
                memcpy(newBuffer.data(), frame->data[0], bufferSize);
            }

            buffer = newBuffer;
            position = 0;

            av_frame_unref(frame.get());
            break;
        }
    }

    return bytes_read > 0 ? bytes_read : -1;
}

auto AudioStreamer::writeData(const char* /* data */, i64 /* size */) -> i64 {
    return -1;
}

auto AudioStreamer::duration() const -> u16 {
    // Duration of streamed audio in seconds.
    return secondsDuration;
}

auto AudioStreamer::format() const -> QAudioFormat {
    return audioFormat;
}

auto AudioStreamer::pos() const -> i64 {
    // Position within a current buffer, not within all the available bytes.
    return position;
}

auto AudioStreamer::size() const -> i64 {
    return availableBytes;
}

auto AudioStreamer::bytesRead() const -> i64 {
    // Number of the all read bytes from all buffers.
    return numBytesRead - position;
}

auto AudioStreamer::bytesAvailable() const -> i64 {
    // Bytes left available.
    return availableBytes - position;
}

auto AudioStreamer::reset() -> bool {
    close();

    formatContext.reset();
    codecContext.reset();
    swrContext.reset();
    packet.reset();
    frame.reset();
    buffer.clear();

    audioStreamIndex = 0;
    position = 0;
    secondsDuration = 0;
    availableBytes = 0;
    channels = 0;
    sampleRate = 0;
    frames = 0;
    numBytesRead = 0;

    return true;
}

auto AudioStreamer::seek(i64 sec) -> bool {
    i64 timestamp = av_rescale(
        sec,
        formatContext->streams[audioStreamIndex]->time_base.den,
        formatContext->streams[audioStreamIndex]->time_base.num
    );

    avformat_seek_file(
        formatContext.get(),
        audioStreamIndex,
        0,
        timestamp,
        timestamp,
        AVSEEK_FLAG_ANY
    );

    buffer.clear();
    position = 0;
    numBytesRead = sec * codecContext->sample_rate *
                   codecContext->ch_layout.nb_channels * SAMPLE_SIZE;

    return true;
}
