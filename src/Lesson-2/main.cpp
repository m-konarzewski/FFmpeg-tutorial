extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
}

#include <format>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("usage: {} <input file>\n", argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    AVFormatContext* fmt_ctx = nullptr;

    int ret = avformat_open_input(&fmt_ctx, input_path, nullptr, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << std::format("avformat_open_input failed: {}\n", err_buf);
        return 1;
    }

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, err_buf, sizeof(err_buf));
        std::cerr << std::format("avformat_find_stream_info failed: {}\n", err_buf);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    std::cout << std::format("Input: {}\n", input_path);
    std::cout << std::format("Format: {} ({})\n", fmt_ctx->iformat->name, fmt_ctx->iformat->long_name);

    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        double duration_sec = static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE;
        std::cout << std::format("Duration: {:.2f} s\n", duration_sec);
    } else {
        std::cout << "Duration: unknown\n";
    }

    if (fmt_ctx->bit_rate > 0) {
        std::cout << std::format("Overall bitrate: {} kb/s\n", fmt_ctx->bit_rate / 1000);
    }

    std::cout << std::format("Streams: {}\n\n", fmt_ctx->nb_streams);

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        std::cout << std::format("Stream #{}:\n", i);

        AVStream* stream = fmt_ctx->streams[i];
        AVCodecParameters* params = stream->codecpar;
        const char* codec_name = avcodec_get_name(params->codec_id);

        if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
            double fps = av_q2d(stream->avg_frame_rate);
            const char* pix_fmt_name = av_get_pix_fmt_name(static_cast<AVPixelFormat>(params->format));

            std::cout << "  Type: video\n";
            std::cout << std::format("  Codec: {}\n", codec_name);
            std::cout << std::format("  Resolution: {}x{}\n", params->width, params->height);
            std::cout << std::format("  Pixel format: {}\n", pix_fmt_name ? pix_fmt_name : "unknown");
            std::cout << std::format("  Frame rate: {:.2f} fps\n", fps);
        } else if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
            const char* sample_fmt_name = av_get_sample_fmt_name(static_cast<AVSampleFormat>(params->format));
            char layout_desc[64];
            av_channel_layout_describe(&params->ch_layout, layout_desc, sizeof(layout_desc));

            std::cout << "  Type: audio\n";
            std::cout << std::format("  Codec: {}\n", codec_name);
            std::cout << std::format("  Sample format: {}\n", sample_fmt_name ? sample_fmt_name : "unknown");
            std::cout << std::format("  Sample rate: {} Hz\n", params->sample_rate);
            std::cout << std::format("  Channels: {} ({})\n", params->ch_layout.nb_channels, layout_desc);
        } else {
            std::cout << std::format("  Type: other ({})\n", static_cast<int>(params->codec_type));
        }

        if (params->bit_rate > 0) {
            std::cout << std::format("  Bitrate: {} kb/s\n", params->bit_rate / 1000);
        }

        std::cout << "\n";
    }

    avformat_close_input(&fmt_ctx);
}