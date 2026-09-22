extern "C" {
#include <libavformat/avformat.h>
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

    int video_stream_index = -1;
    int audio_stream_index = -1;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
        } else if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = static_cast<int>(i);
        }
    }

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "av_packet_alloc failed\n";
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    int packet_count = 0;
    int video_packet_count = 0;
    int audio_packet_count = 0;

    while (true) {
        int ret = av_read_frame(fmt_ctx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                std::cout << "EOF\n";
                break;
            } else {
                char err_buf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, err_buf, sizeof(err_buf));
                std::cerr << std::format("av_read_frame failed: {}\n", err_buf);
                break;
            }
        }

        if (packet->stream_index == video_stream_index) {
            ++video_packet_count;
            const char* keyframe = (packet->flags & AV_PKT_FLAG_KEY) ? "yes" : "no";
            std::cout << std::format("Video packet #{}: pts={} dts={} size={} bytes, keyframe={}\n", video_packet_count,
                                     packet->pts, packet->dts, packet->size, keyframe);
        } else if (packet->stream_index == audio_stream_index) {
            ++audio_packet_count;
            std::cout << std::format("Audio packet #{}: pts={} dts={} size={} bytes\n", audio_packet_count, packet->pts,
                                     packet->dts, packet->size);
        }

        ++packet_count;
        av_packet_unref(packet);
    }

    std::cout << std::format("\nTotal packets read: {}\n", packet_count);
    std::cout << std::format("Video packets: {}\n", video_packet_count);
    std::cout << std::format("Audio packets: {}\n", audio_packet_count);

    av_packet_free(&packet);
    avformat_close_input(&fmt_ctx);
}