#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <omp.h>

#include <QBuffer>

#include "type_aliases.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

constexpr u8 BANDS = 10;
using namespace juce::dsp::IIR;

inline auto getDuration(const u16 chn, const u32 sampleRate,
                        const vector<f32>& data) -> u32 {
    return data.size() / (static_cast<u32>(chn * sampleRate));
}

auto extractSamples(const path& filename)
    -> tuple<AVCodecContext, u32, vector<f32>> {
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

    const u16 sampleRate = codec_ctx->sample_rate;
    const auto channelLayout = codec_ctx->ch_layout;
    const u16 channels = codec_ctx->ch_layout.nb_channels;
    const i64 frames = codec_ctx->frame_num;

    AVChannelLayout layout;
    av_channel_layout_default(&layout, codec_ctx->ch_layout.nb_channels);

    struct SwrContext* swr_ctx = nullptr;
    const i32 ret = swr_alloc_set_opts2(
        &swr_ctx, &layout, AV_SAMPLE_FMT_FLT, sampleRate, &channelLayout,
        codec_ctx->sample_fmt, sampleRate, 0, nullptr);

    if (ret < 0) {
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        throw panic("Error allocating SwrContext.");
    }

    swr_init(swr_ctx);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    vector<f32> samples;

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    const u16 samplesNumber = frame->nb_samples;

                    vector<f32> temp_buffer(
                        static_cast<u32>(samplesNumber * channels));
                    u8* outBuffers[1] = {
                        reinterpret_cast<u8*>(temp_buffer.data())};

                    const u16 convertedSamples =
                        swr_convert(swr_ctx, outBuffers, samplesNumber,
                                    frame->data, samplesNumber);

                    if (convertedSamples > 0) {
                        samples.insert(samples.end(), temp_buffer.begin(),
                                       temp_buffer.begin() +
                                           (static_cast<u32>(convertedSamples *
                                                             channels)));
                    }

                    av_frame_unref(frame);
                }
            }
        }

        av_packet_unref(packet);
    }

    const auto ctx = *codec_ctx;

    av_packet_free(&packet);
    av_frame_free(&frame);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    const u32 duration = getDuration(channels, sampleRate, samples);

    return {ctx, duration, samples};
}

inline auto makeAudioBuffer(const vector<f32>& audioData) -> QBuffer* {
    auto* qBuffer = new QBuffer();
    qBuffer->setData(reinterpret_cast<const char*>(audioData.data()),
                     static_cast<i32>(audioData.size() * sizeof(f32)));
    qBuffer->open(QIODevice::ReadOnly);
    return qBuffer;
}

inline auto equalizeAudioFile(const path& inputFile,
                              const array<f32, BANDS>& gains)
    -> tuple<AVCodecContext, u32, QBuffer*> {
    auto [context, _duration, audioData] = extractSamples(inputFile);
    if (audioData.empty()) {
        return {};
    }

    const u32 sampleRate = context.sample_rate;
    const u16 channels = context.ch_layout.nb_channels;
    const i64 frames = context.frame_num;

    constexpr array<f32, BANDS> centerFrequencies = {
        31.0,   62.0,   125.0,  250.0,  500.0,
        1000.0, 2000.0, 4000.0, 8000.0, 16000.0};
    constexpr f32 QFactor = 1.0;

    vector<array<Filter<f32>, BANDS>> filters(channels);

#pragma omp parallel for collapse(2)
    for (i32 ch = 0; ch < channels; ch++) {
        for (u8 band = 0; band < BANDS; band++) {
            filters[ch][band].coefficients = Coefficients<f32>::makePeakFilter(
                sampleRate, centerFrequencies[band], QFactor, gains[band]);
            filters[ch][band].reset();
        }
    }

    juce::AudioBuffer<f32> buffer(channels, static_cast<i32>(frames));

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(buffer.getWritePointer(ch), &audioData[ch * frames],
               frames * sizeof(f32));
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        f32* ptr = buffer.getWritePointer(ch);
        f32* const* ref = &ptr;

        juce::dsp::AudioBlock<f32> channelBlock(ref, 1, frames);
        juce::dsp::ProcessContextReplacing<f32> context(channelBlock);

        for (u8 band = 0; band < BANDS; band++) {
            filters[ch][band].process(context);
        }
    }

#pragma omp parallel for
    for (i32 ch = 0; ch < channels; ch++) {
        memcpy(&audioData[ch * frames], buffer.getReadPointer(ch),
               frames * sizeof(f32));
    }

    const u32 duration = getDuration(channels, sampleRate, audioData);
    return {context, duration, makeAudioBuffer(audioData)};
}

inline auto getAudioFile(const path& inputFile)
    -> tuple<AVCodecContext, u32, QBuffer*> {
    auto [context, duration, audioData] = extractSamples(inputFile);
    return {context, duration, makeAudioBuffer(audioData)};
}