#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <omp.h>

#include <QAudioSink>
#include <QBuffer>

#include "constants.hpp"
#include "mainwindow.hpp"
#include "type_aliases.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

struct AudioMetadata {
    const u8 channels;
    const u16 sampleRate;
    const u32 frames;
};

inline void playTrack(const path& filename, MainWindow* window) {
    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, filename.string().c_str(), nullptr,
                            nullptr) < 0) {
        throw panic("Failed to open input file.");
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        throw panic("Failed to find stream info.");
    }

    const AVCodec* codec = nullptr;
    const i32 audio_stream_idx =
        av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audio_stream_idx < 0) {
        avformat_close_input(&format_ctx);
        throw panic("No audio stream found.");
    }

    const AVStream* audio_stream = format_ctx->streams[audio_stream_idx];
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->thread_count = 0;
    codec_ctx->thread_type = FF_THREAD_FRAME;

    avcodec_parameters_to_context(codec_ctx, audio_stream->codecpar);
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        throw panic("Failed to open codec.");
    }

    const auto channelLayout = codec_ctx->ch_layout;
    const u8 channels = channelLayout.nb_channels;
    const u16 sampleRate = codec_ctx->sample_rate;
    const u32 frames = codec_ctx->frame_num;

    AVChannelLayout layout;
    av_channel_layout_default(&layout, channels);

    SwrContext* swr_ctx = nullptr;
    if (swr_alloc_set_opts2(&swr_ctx, &layout, AV_SAMPLE_FMT_S16, sampleRate,
                            &channelLayout, codec_ctx->sample_fmt, sampleRate,
                            0, nullptr) < 0) {
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        throw panic("Error allocating SwrContext.");
    }

    swr_init(swr_ctx);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    const u32 secondsDuration = format_ctx->duration / AV_TIME_BASE;

    auto* newAudioBytes = new QByteArray();
    newAudioBytes->resize(
        static_cast<u32>(static_cast<u32>(channels * sampleRate) *
                         secondsDuration * SAMPLE_SIZE));
    auto* newAudioBuffer = new QBuffer(newAudioBytes);

    u32 samplesIdx = 0;
    bool firstFrameDecoded = false;

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (avcodec_send_packet(codec_ctx, packet) == 0) {
            while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                const u16 samplesNumber = frame->nb_samples;
                const u16 dataSize = samplesNumber * channels * SAMPLE_SIZE;
                u8* output =
                    reinterpret_cast<u8*>(newAudioBytes->data() + samplesIdx);

                if (codec_ctx->sample_fmt != AV_SAMPLE_FMT_S16) {
                    AVFrame* newFrame = av_frame_alloc();
                    newFrame->ch_layout = channelLayout;
                    newFrame->sample_rate = sampleRate;
                    newFrame->format = AV_SAMPLE_FMT_S16;
                    swr_convert_frame(swr_ctx, newFrame, frame);
                    memcpy(output, newFrame->data[0], dataSize);
                    av_frame_free(&newFrame);
                } else {
                    memcpy(output, frame->data[0], dataSize);
                }

                samplesIdx += dataSize;

                if (!firstFrameDecoded) {
                    firstFrameDecoded = true;

                    QMetaObject::invokeMethod(window, [&]() {
                        if (window->audioSink) {
                            window->audioSink->stop();
                        }

                        window->audioBytes = newAudioBytes;
                        window->audioBuffer = newAudioBuffer;
                        window->audioBuffer->open(QIODevice::ReadOnly);

                        window->audioSink = new QAudioSink(format, window);
                        window->audioSink->start(window->audioBuffer);

                        window->audioDuration =
                            MainWindow::toMinutes(secondsDuration);
                        window->audioBytesNum = static_cast<u32>(
                            SAMPLE_SIZE * channels * sampleRate);
                        window->progressSlider->setRange(0, secondsDuration);
                    });
                }
            }
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
}