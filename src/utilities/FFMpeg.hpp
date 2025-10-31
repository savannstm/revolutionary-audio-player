#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/tx.h>
}

#include "Aliases.hpp"

namespace FFmpeg {
    struct FormatContextDeleter {
        void operator()(AVFormatContext* ctx) const {
            avformat_close_input(&ctx);
        }
    };

    struct CodecContextDeleter {
        void operator()(AVCodecContext* ctx) const {
            avcodec_free_context(&ctx);
        }
    };

    struct PacketDeleter {
        void operator()(AVPacket* pkt) const { av_packet_free(&pkt); }
    };

    struct FrameDeleter {
        void operator()(AVFrame* frame) const { av_frame_free(&frame); }
    };

    struct FilterGraphDeleter {
        void operator()(AVFilterGraph* filterGraph) const {
            avfilter_graph_free(&filterGraph);
        }
    };

    struct TXContextDeleter {
        void operator()(AVTXContext* txContext) const {
            av_tx_uninit(&txContext);
        }
    };

    using FormatContext = unique_ptr<AVFormatContext, FormatContextDeleter>;
    using CodecContext = unique_ptr<AVCodecContext, CodecContextDeleter>;
    using Packet = unique_ptr<AVPacket, PacketDeleter>;
    using Frame = unique_ptr<AVFrame, FrameDeleter>;
    using FilterGraph = unique_ptr<AVFilterGraph, FilterGraphDeleter>;
    using TXContext = unique_ptr<AVTXContext, TXContextDeleter>;

    inline auto createPacket() -> Packet {
        return Packet(av_packet_alloc());
    }

    inline auto createFrame() -> Frame {
        return Frame(av_frame_alloc());
    }

    inline auto createCodecContext(const AVCodec* codec) -> CodecContext {
        return CodecContext(avcodec_alloc_context3(codec));
    }

    inline auto createFilterGraph() -> FilterGraph {
        return FilterGraph(avfilter_graph_alloc());
    }

    inline auto createTXContext() -> TXContext {
        return {};
    }

}  // namespace FFmpeg