#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <omp.h>
#include <qelapsedtimer.h>

#include <QBuffer>

#include "type_aliases.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

constexpr u8 BANDS_NUMBER = 10;
constexpr u32 BUFFER_CHUNK = 4096;

struct AudioMetadata {
    const u8 channels;
    const u16 sampleRate;
    const u32 frames;
};

inline auto extractSamples(const path& filename, vector<i16>& samples)
    -> tuple<AudioMetadata, u32> {
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

    const u32 duration = format_ctx->duration / AV_TIME_BASE;

    samples.resize((sampleRate * channels * duration));
    samples.clear();

    vector<i16> temp_buffer;
    temp_buffer.resize((BUFFER_CHUNK * channels));
    u8* outBuffers[1] = {reinterpret_cast<u8*>(temp_buffer.data())};

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    const u16 samplesNumber = frame->nb_samples;

                    const u16 convertedSamples =
                        swr_convert(swr_ctx, outBuffers, samplesNumber,
                                    frame->data, samplesNumber);

                    if (convertedSamples > 0) {
                        samples.insert(
                            samples.end(), temp_buffer.begin(),
                            temp_buffer.begin() + convertedSamples * channels);
                    }

                    av_frame_unref(frame);
                }
            }
        }

        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    const auto metadata = AudioMetadata{
        .channels = channels, .sampleRate = sampleRate, .frames = frames};

    return {metadata, duration};
}