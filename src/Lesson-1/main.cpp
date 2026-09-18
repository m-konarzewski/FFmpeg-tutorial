#include <iostream>

#include <SDL3/SDL.h>

extern "C" {
#include <libavutil/avutil.h>
}

int main() {
    std::cout << "FFmpeg libavutil: " << av_version_info() << '\n';
    std::cout << "SDL3: " << SDL_GetVersion() << '\n';
}