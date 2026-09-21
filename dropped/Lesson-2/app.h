// application.h
#pragma once

#include <optional>
#include <stdexcept>

#include <SDL3/SDL.h>

#include "sdl_window.h"

class Application {
   public:
    Application() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }

        try {
            window.emplace("FFmpeg tutorial", SDL_WINDOW_RESIZABLE);
        } catch (...) {
            SDL_Quit();
            throw;
        }
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    ~Application() {
        SDL_Quit();
    }

    void Run() {
        bool running = true;
        while (running) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
            }

            window.value().Show();
        }
    }

   private:
    std::optional<SdlWindow> window;
};