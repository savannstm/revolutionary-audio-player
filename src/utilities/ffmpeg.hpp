#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "aliases.hpp"

namespace FFmpeg {
    struct FormatContextDeleter {
        void operator()(AVFormatContext* ctx) const {
            avformat_close_input(&ctx);
        }
    };

    struct CodecContextDeleter {
        void operator()(AVCodecContext* ctx) const {
            avcodec_flush_buffers(ctx);
            avcodec_free_context(&ctx);
        }
    };

    struct SwrContextDeleter {
        void operator()(SwrContext* ctx) const { swr_free(&ctx); }
    };

    struct PacketDeleter {
        void operator()(AVPacket* pkt) const {
            av_packet_unref(pkt);
            av_packet_free(&pkt);
        }
    };

    struct FrameDeleter {
        void operator()(AVFrame* frame) const {
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
    };

    using FormatContextPtr = unique_ptr<AVFormatContext, FormatContextDeleter>;
    using CodecContextPtr = unique_ptr<AVCodecContext, CodecContextDeleter>;
    using SwrContextPtr = unique_ptr<SwrContext, SwrContextDeleter>;
    using PacketPtr = unique_ptr<AVPacket, PacketDeleter>;
    using FramePtr = unique_ptr<AVFrame, FrameDeleter>;

    inline auto make_packet() -> PacketPtr {
        return PacketPtr(av_packet_alloc());
    }

    inline auto make_frame() -> FramePtr {
        return FramePtr(av_frame_alloc());
    }

    inline auto make_codec_context(const AVCodec* codec) -> CodecContextPtr {
        return CodecContextPtr(avcodec_alloc_context3(codec));
    }
}  // namespace FFmpeg