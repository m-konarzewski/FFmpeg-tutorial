#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>

#include <SDL3/SDL.h>

#include "ffmpeg_handles.h"
#include "sdl_texture.h"
#include "video_decoder.h"

class VideoPlayer
{
    static constexpr AVRational kNanosecondTimeBase{1, 1'000'000'000};

public:
    VideoPlayer(const char* path, SDL_Renderer* renderer) : decoder(path)
    {
        if (decoder.PixelFormat() != AV_PIX_FMT_YUV420P) {
            const char* name = av_get_pix_fmt_name(decoder.PixelFormat());
            throw std::runtime_error{std::format("Unsupported pixel format: {}", name ? name : "unknown")};
        }

        texture.reset(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, decoder.Width(),
                                        decoder.Height()));
        if (!texture) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTexture failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }
    }

    int Width() const
    {
        return decoder.Width();
    }

    int Height() const
    {
        return decoder.Height();
    }

    SDL_Texture* Texture() const
    {
        return hasFrame ? texture.get() : nullptr;
    }

    void Update()
    {
        if (!pending) {
            if (finished) {
                return;
            }

            pending = decoder.NextFrame();
            if (!pending) {
                finished = true;
                SDL_Log("End of stream");
                return;
            }

            if (!started) {
                started = true;
                const int64_t pts = Pts(*pending);
                firstPts = pts != AV_NOPTS_VALUE ? pts : 0;
                startTimeNs = SDL_GetTicksNS();
            }
        }

        if (SDL_GetTicksNS() - startTimeNs < DueTimeNs(*pending)) {
            return;
        }

        Upload(*pending);
        pending = nullptr;
    }

private:
    static int64_t Pts(const AVFrame& frame)
    {
        return frame.pts != AV_NOPTS_VALUE ? frame.pts : frame.best_effort_timestamp;
    }

    uint64_t DueTimeNs(const AVFrame& frame) const
    {
        const int64_t pts = Pts(frame);
        if (pts == AV_NOPTS_VALUE) {
            return 0;
        }

        const int64_t offset = std::max<int64_t>(pts - firstPts, 0);
        return static_cast<uint64_t>(av_rescale_q(offset, decoder.TimeBase(), kNanosecondTimeBase));
    }

    void Upload(const AVFrame& frame)
    {
        if (!SDL_UpdateYUVTexture(texture.get(), nullptr, frame.data[0], frame.linesize[0], frame.data[1],
                                  frame.linesize[1], frame.data[2], frame.linesize[2])) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_UpdateYUVTexture failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }
        hasFrame = true;
    }

    VideoDecoder decoder;
    SdlTexture texture;
    const AVFrame* pending = nullptr;
    bool started = false;
    bool finished = false;
    bool hasFrame = false;
    int64_t firstPts = 0;
    uint64_t startTimeNs = 0;
};