#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

// Adapter for FFmpeg's "free" functions, which all share the same shape:
// they take a T** and set the pointee to nullptr after releasing it.
// The function itself is a non-type template parameter, so the deleter is
// stateless and unique_ptr stays the size of a raw pointer.
template <auto Free>
struct Deleter
{
    template <typename T>
    void operator()(T* ptr) const noexcept
    {
        Free(&ptr);
    }
};

template <typename T, auto Free>
using Handle = std::unique_ptr<T, Deleter<Free>>;

using FormatContext = Handle<AVFormatContext, avformat_close_input>;
using CodecContext = Handle<AVCodecContext, avcodec_free_context>;
using Packet = Handle<AVPacket, av_packet_free>;
using Frame = Handle<AVFrame, av_frame_free>;

struct Counters
{
    std::int64_t packets_sent = 0;
    std::int64_t frames_received = 0;
};

std::string error_string(int error)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(error, buffer, sizeof(buffer));
    return buffer;
}

std::string timestamp_string(std::int64_t timestamp)
{
    return timestamp != AV_NOPTS_VALUE ? std::format("{}", timestamp) : std::string{"N/A"};
}

void print_frame(const AVFrame* frame, const AVStream* stream, std::int64_t index)
{
    // Not every demuxer/decoder combination fills frame->pts, so fall back
    // to the timestamp FFmpeg guessed for the frame.
    const std::int64_t pts = frame->pts != AV_NOPTS_VALUE ? frame->pts : frame->best_effort_timestamp;

    const std::string time_text = pts != AV_NOPTS_VALUE
                                      ? std::format("{:.4f}s", static_cast<double>(pts) * av_q2d(stream->time_base))
                                      : std::string{"N/A"};

    const bool is_key = (frame->flags & AV_FRAME_FLAG_KEY) != 0;

    std::cout << std::format("frame #{:<6} pts={:<8} dts={:<8} t={:<11} type={} key={}\n", index, timestamp_string(pts),
                             timestamp_string(frame->pkt_dts), time_text, av_get_picture_type_char(frame->pict_type),
                             is_key ? 1 : 0);
}

// Pulls every frame the decoder can currently give us.
// Returns 0 when the decoder needs more input (EAGAIN) or is fully drained (EOF),
// a negative AVERROR code on a real decoding error.
int receive_frames(AVCodecContext* codec, AVFrame* frame, const AVStream* stream, Counters& counters)
{
    while (true) {
        const int ret = avcodec_receive_frame(codec, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        }
        if (ret < 0) {
            return ret;
        }

        print_frame(frame, stream, counters.frames_received);
        ++counters.frames_received;

        av_frame_unref(frame);
    }
}

int run(const char* path)
{
    // 1. Open the container. avformat_open_input takes an AVFormatContext**
    //    and allocates the context itself, which is exactly the shape
    //    std::out_ptr adapts: the unique_ptr is reset with the new pointer
    //    once the full expression ends (and stays empty if the call failed,
    //    because FFmpeg frees the context and nulls it on error).
    FormatContext format;
    int ret = avformat_open_input(std::out_ptr(format), path, nullptr, nullptr);
    if (ret < 0) {
        std::cerr << std::format("avformat_open_input failed: {}\n", error_string(ret));
        return 1;
    }

    ret = avformat_find_stream_info(format.get(), nullptr);
    if (ret < 0) {
        std::cerr << std::format("avformat_find_stream_info failed: {}\n", error_string(ret));
        return 1;
    }

    // 2. Find the first video stream.
    int video_index = -1;
    for (unsigned i = 0; i < format->nb_streams; ++i) {
        if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = static_cast<int>(i);
            break;
        }
    }
    if (video_index < 0) {
        std::cerr << "No video stream found\n";
        return 1;
    }
    const AVStream* stream = format->streams[video_index];

    // 3. Find a decoder for the stream's codec.
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        std::cerr << std::format("No decoder found for codec '{}'\n", avcodec_get_name(stream->codecpar->codec_id));
        return 1;
    }

    // 4. Create the decoder context (the allocator returns the pointer
    //    directly, so it is handed straight to the unique_ptr) and fill it
    //    from the stream parameters.
    CodecContext codec{avcodec_alloc_context3(decoder)};
    if (!codec) {
        std::cerr << "avcodec_alloc_context3 failed\n";
        return 1;
    }

    ret = avcodec_parameters_to_context(codec.get(), stream->codecpar);
    if (ret < 0) {
        std::cerr << std::format("avcodec_parameters_to_context failed: {}\n", error_string(ret));
        return 1;
    }

    // 5. Open the decoder.
    ret = avcodec_open2(codec.get(), decoder, nullptr);
    if (ret < 0) {
        std::cerr << std::format("avcodec_open2 failed: {}\n", error_string(ret));
        return 1;
    }

    // Resolution and pixel format are properties of the stream, not of a
    // single frame, so they are printed once here.
    const char* pixel_format = av_get_pix_fmt_name(codec->pix_fmt);
    std::cout << std::format("Decoder: {} ({}), {}x{}, {}\n\n", decoder->name,
                             decoder->long_name ? decoder->long_name : "", codec->width, codec->height,
                             pixel_format ? pixel_format : "unknown");

    // 6. Allocate the reusable packet and frame.
    Packet packet{av_packet_alloc()};
    Frame frame{av_frame_alloc()};
    if (!packet || !frame) {
        std::cerr << "Failed to allocate packet or frame\n";
        return 1;
    }

    Counters counters;

    // 7. Demux -> send packet -> receive frames.
    while (true) {
        ret = av_read_frame(format.get(), packet.get());
        if (ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            std::cerr << std::format("av_read_frame failed: {}\n", error_string(ret));
            return 1;
        }

        if (packet->stream_index != video_index) {
            av_packet_unref(packet.get());
            continue;
        }

        ret = avcodec_send_packet(codec.get(), packet.get());
        av_packet_unref(packet.get());
        if (ret < 0) {
            std::cerr << std::format("avcodec_send_packet failed: {}\n", error_string(ret));
            return 1;
        }
        ++counters.packets_sent;

        ret = receive_frames(codec.get(), frame.get(), stream, counters);
        if (ret < 0) {
            std::cerr << std::format("avcodec_receive_frame failed: {}\n", error_string(ret));
            return 1;
        }
    }

    // 8. Drain the decoder: a null packet signals end of stream and makes the
    //    decoder release the frames it is still holding internally.
    ret = avcodec_send_packet(codec.get(), nullptr);
    if (ret < 0) {
        std::cerr << std::format("avcodec_send_packet (flush) failed: {}\n", error_string(ret));
        return 1;
    }

    ret = receive_frames(codec.get(), frame.get(), stream, counters);
    if (ret < 0) {
        std::cerr << std::format("avcodec_receive_frame (flush) failed: {}\n", error_string(ret));
        return 1;
    }

    std::cout << std::format("\nPackets sent: {}, frames decoded: {}\n", counters.packets_sent,
                             counters.frames_received);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << std::format("Usage: {} <input file>\n", argv[0]);
        return 1;
    }

    return run(argv[1]);
}