#include "audiostreamer.hpp"

#include "aliases.hpp"
#include "constants.hpp"
#include "mainwindow.hpp"

#include "libavutil/frame.h"
#include "libavutil/rational.h"

#include <algorithm>
#include <cstdint>

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
        qWarning() << std::format(
            "Failed to open input file: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
    }

    formatContext.reset(formatContextPtr);
    formatContextPtr = formatContext.get();

    err = avformat_find_stream_info(formatContextPtr, nullptr);

    if (err < 0) {
        qWarning() << std::format(
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
        qWarning() << std::format(
            "No audio stream found: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
    }

    const auto* audioStream = formatContextPtr->streams[audioStreamIndex];
    codecContext = make_codec_context(codec);
    codecContext->thread_count = 0;
    codecContext->thread_type = FF_THREAD_FRAME;

    AVCodecContext* codecContextPtr = codecContext.get();
    avcodec_parameters_to_context(codecContextPtr, audioStream->codecpar);

    err = avcodec_open2(codecContextPtr, codec, nullptr);

    if (err < 0) {
        qWarning() << std::format(
            "Failed to open codec: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
    }

    const AVChannelLayout channelLayout = codecContext->ch_layout;
    const u8 channelNumber = channelLayout.nb_channels;
    const u16 sampleRate = codecContext->sample_rate;
    const u32 frames = codecContext->frame_num;

    auto* swrContextPtr = swrContext.get();

    err = swr_alloc_set_opts2(
        &swrContextPtr,
        &channelLayout,
        AV_SAMPLE_FMT_S16,
        sampleRate,
        &channelLayout,
        codecContext->sample_fmt,
        sampleRate,
        0,
        nullptr
    );

    if (err < 0) {
        qWarning() << std::format(
            "Error allocating SwrContext: {}",
            av_make_error_string(errBuf.data(), AV_ERROR_MAX_STRING_SIZE, err)
        );
        return false;
    }

    swrContext.reset(swrContextPtr);
    swrContextPtr = swrContext.get();

    swr_init(swrContextPtr);

    packet = make_packet();
    frame = make_frame();

    secondsDuration = formatContextPtr->duration / AV_TIME_BASE;
    bytesPerSecond = static_cast<i64>(sampleRate * channelNumber * SAMPLE_SIZE);

    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelCount(channelNumber);

    return open(QIODevice::ReadOnly);
}

auto AudioStreamer::readData(char* data, const qi64 maxSize) -> qi64 {
    i64 bytesRead = 0;

    while (bytesRead < maxSize) {
        if (bufferPosition < buffer.size()) {
            const u32 chunkSize =
                std::min(maxSize - bytesRead, buffer.size() - bufferPosition);

            memcpy(
                data + bytesRead,
                buffer.constData() + bufferPosition,
                chunkSize
            );
            bufferPosition += chunkSize;
            bytesRead += chunkSize;

            if (bufferPosition >= buffer.size()) {
                buffer.clear();
                bufferPosition = 0;

                auto* window = static_cast<MainWindow*>(parent());
                QMetaObject::invokeMethod(window, &MainWindow::updateProgress);
            }

            continue;
        }

        if (av_read_frame(formatContext.get(), packet.get()) < 0) {
            ended = true;
            auto* window = static_cast<MainWindow*>(parent());
            QMetaObject::invokeMethod(window, &MainWindow::eof);
            break;
        }

        if (packet->stream_index != audioStreamIndex) {
            av_packet_unref(packet.get());
            continue;
        }

        i32 err = avcodec_send_packet(codecContext.get(), packet.get());
        av_packet_unref(packet.get());

        if (err < 0) {
            ended = true;
            auto* window = static_cast<MainWindow*>(parent());
            QMetaObject::invokeMethod(window, &MainWindow::eof);
            break;
        }

        while (err >= 0) {
            err = avcodec_receive_frame(codecContext.get(), frame.get());

            if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
                if (err == AVERROR_EOF) {
                    ended = true;
                    auto* window = static_cast<MainWindow*>(parent());
                    QMetaObject::invokeMethod(window, &MainWindow::eof);
                }

                av_frame_unref(frame.get());
                break;
            }

            if (err < 0) {
                av_frame_unref(frame.get());
                return -1;
            }

            const u16 bufferSize = frame->nb_samples *
                                   codecContext->ch_layout.nb_channels *
                                   SAMPLE_SIZE;

            QByteArray newBuffer = QByteArray();
            newBuffer.resize(bufferSize);
            byteOffset += bufferSize;

            playbackSecond = static_cast<u16>(
                static_cast<f64>(frame->pts) *
                av_q2d(formatContext->streams[audioStreamIndex]->time_base)
            );

            if (codecContext->sample_fmt != AV_SAMPLE_FMT_S16) {
                FramePtr newFrame = make_frame();
                newFrame->ch_layout = codecContext->ch_layout;
                newFrame->sample_rate = codecContext->sample_rate;
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
            bufferPosition = 0;

            av_frame_unref(frame.get());
            break;
        }
    }

    return bytesRead > 0 ? bytesRead : -1;
}

// Not for write
auto AudioStreamer::writeData(const char* /* data */, qi64 /* size */) -> qi64 {
    return -1;
}

auto AudioStreamer::duration() const -> u16 {
    return secondsDuration;
}

auto AudioStreamer::format() const -> QAudioFormat {
    return audioFormat;
}

auto AudioStreamer::pos() const -> qi64 {
    return byteOffset - (buffer.size() - bufferPosition);
}

// Buffer supports seeking.
auto AudioStreamer::isSequential() const -> bool {
    return false;
}

auto AudioStreamer::second() const -> u16 {
    return playbackSecond;
}

auto AudioStreamer::atEnd() const -> bool {
    return ended;
}

// Number of available bytes cannot be properly determined before decoding,
// especially if source is variable-bitrate.
// Just let it assume there's a lot of bytes.
auto AudioStreamer::bytesAvailable() const -> qi64 {
    return ended ? 0 : INT64_MAX;
}

auto AudioStreamer::reset() -> bool {
    QIODevice::close();

    formatContext.reset();
    codecContext.reset();
    swrContext.reset();
    packet.reset();
    frame.reset();

    buffer.clear();

    ended = false;

    audioStreamIndex = 0;
    bufferPosition = 0;
    bytesPerSecond = 0;
    secondsDuration = 0;
    byteOffset = 0;

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

    buffer.clear();
    bufferPosition = 0;
    byteOffset = second * bytesPerSecond;

    return true;
}
