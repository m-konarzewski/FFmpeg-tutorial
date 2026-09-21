// sdl_window.h
#pragma once

#include <stdexcept>
#include <utility>

#include <SDL3/SDL.h>

class SdlWindow {
    static constexpr int kWindowWidth = 1440;
    static constexpr int kWindowHeight = 810;

   public:
    SdlWindow(const char* title, SDL_WindowFlags flags) {
        window = SDL_CreateWindow(title, kWindowWidth, kWindowHeight, flags);
        if (!window) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s", SDL_GetError());
            SDL_DestroyWindow(window);
            throw std::runtime_error{SDL_GetError()};
        }
    }

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;

    SdlWindow(SdlWindow&& other) noexcept
        : window(std::exchange(other.window, nullptr)),
          renderer(std::exchange(other.renderer, nullptr)) {}

    SdlWindow& operator=(SdlWindow&& other) noexcept {
        if (this != &other) {
            Destroy();
            window = std::exchange(other.window, nullptr);
            renderer = std::exchange(other.renderer, nullptr);
        }
        return *this;
    }

    ~SdlWindow() {
        Destroy();
    }

    void Show() {
        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

   private:
    void Destroy() noexcept {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
};
