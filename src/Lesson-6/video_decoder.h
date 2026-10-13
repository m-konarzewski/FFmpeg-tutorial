// video_decoder.h
#pragma once

#include <format>
#include <memory>
#include <stdexcept>

#include "ffmpeg_handles.h"

// Pull-based video decoder: every NextFrame() call returns the next decoded
// frame in presentation order, or nullptr once the whole file has been decoded.
class VideoDecoder
{
public:
    explicit VideoDecoder(const char* path)
    {
        Check(avformat_open_input(std::out_ptr(format), path, nullptr, nullptr), "avformat_open_input");
        Check(avformat_find_stream_info(format.get(), nullptr), "avformat_find_stream_info");

        for (unsigned i = 0; i < format->nb_streams; ++i) {
            if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoIndex = static_cast<int>(i);
                break;
            }
        }
        if (videoIndex < 0) {
            throw std::runtime_error{"No video stream found"};
        }

        const AVStream* stream = format->streams[videoIndex];
        timeBase = stream->time_base;

        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder) {
            throw std::runtime_error{
                std::format("No decoder found for codec '{}'", avcodec_get_name(stream->codecpar->codec_id))};
        }

        codec.reset(avcodec_alloc_context3(decoder));
        if (!codec) {
            throw std::runtime_error{"avcodec_alloc_context3 failed"};
        }
        Check(avcodec_parameters_to_context(codec.get(), stream->codecpar), "avcodec_parameters_to_context");
        Check(avcodec_open2(codec.get(), decoder, nullptr), "avcodec_open2");

        packet.reset(av_packet_alloc());
        frame.reset(av_frame_alloc());
        if (!packet || !frame) {
            throw std::runtime_error{"Failed to allocate packet or frame"};
        }
    }

    int Width() const
    {
        return codec->width;
    }

    int Height() const
    {
        return codec->height;
    }

    AVPixelFormat PixelFormat() const
    {
        return codec->pix_fmt;
    }

    AVRational TimeBase() const
    {
        return timeBase;
    }

    const AVFrame* NextFrame()
    {
        while (true) {
            const int ret = avcodec_receive_frame(codec.get(), frame.get());
            if (ret == 0) {
                return frame.get();
            }
            if (ret == AVERROR_EOF) {
                return nullptr;
            }
            if (ret != AVERROR(EAGAIN)) {
                Check(ret, "avcodec_receive_frame");
            }

            FeedDecoder();
        }
    }

private:
    static void Check(int ret, const char* what)
    {
        if (ret < 0) {
            throw std::runtime_error{std::format("{} failed: {}", what, error_string(ret))};
        }
    }

    void FeedDecoder()
    {
        while (true) {
            int ret = av_read_frame(format.get(), packet.get());
            if (ret == AVERROR_EOF) {
                Check(avcodec_send_packet(codec.get(), nullptr), "avcodec_send_packet (flush)");
                return;
            }
            Check(ret, "av_read_frame");

            if (packet->stream_index != videoIndex) {
                av_packet_unref(packet.get());
                continue;
            }

            ret = avcodec_send_packet(codec.get(), packet.get());
            av_packet_unref(packet.get());
            Check(ret, "avcodec_send_packet");
            return;
        }
    }

    FormatContext format;
    CodecContext codec;
    Packet packet;
    Frame frame;
    int videoIndex = -1;
    AVRational timeBase{};
};