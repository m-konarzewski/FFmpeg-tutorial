// sdl_window.h
#pragma once

#include <stdexcept>
#include <utility>

#include <SDL3/SDL.h>

class SdlWindow
{
    static constexpr int kWindowWidth = 1440;
    static constexpr int kWindowHeight = 810;

public:
    SdlWindow(const char* title, SDL_WindowFlags flags)
    {
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

        if (!SDL_SetRenderVSync(renderer, 1)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SDL_SetRenderVSync failed: %s", SDL_GetError());
        }
    }

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&& other) noexcept = delete;
    SdlWindow& operator=(SdlWindow&& other) = delete;

    ~SdlWindow()
    {
        Destroy();
    }

    SDL_Renderer* Renderer() const
    {
        return renderer;
    }

    void SetLogicalSize(int width, int height)
    {
        if (!SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_SetRenderLogicalPresentation failed: %s", SDL_GetError());
            throw std::runtime_error{SDL_GetError()};
        }
    }

    void Show(SDL_Texture* texture)
    {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        if (texture) {
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        }
        SDL_RenderPresent(renderer);
    }

private:
    void Destroy() noexcept
    {
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