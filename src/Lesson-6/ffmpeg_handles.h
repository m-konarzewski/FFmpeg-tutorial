#pragma once

#include <memory>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

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

inline std::string error_string(int error)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(error, buffer, sizeof(buffer));
    return buffer;
}
