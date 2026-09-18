#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/avutil.h>
}

#include <iostream>

int main()
{
    std::cout << "FFmpeg libavutil: " << av_version_info() << '\n';
    std::cout << "SDL3: " << SDL_GetVersion() << '\n';
}