#pragma once

#include <optional>
#include <stdexcept>

#include <SDL3/SDL.h>

#include "sdl_window.h"
#include "video_player.h"

class Application
{
public:
    explicit Application(const char* videoPath)
    {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }

        try {
            window.emplace("FFmpeg tutorial", SDL_WINDOW_RESIZABLE);
            player.emplace(videoPath, window.value().Renderer());
            window.value().SetLogicalSize(player.value().Width(), player.value().Height());
        } catch (...) {
            Shutdown();
            throw;
        }
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    ~Application()
    {
        Shutdown();
    }

    void Run()
    {
        bool running = true;
        while (running) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
            }

            player.value().Update();
            window.value().Show(player.value().Texture());
        }
    }

private:
    void Shutdown() noexcept
    {
        player.reset();
        window.reset();
        SDL_Quit();
    }

    std::optional<SdlWindow> window;
    std::optional<VideoPlayer> player;
};
