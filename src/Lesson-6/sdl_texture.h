#pragma once

#include <memory>

#include <SDL3/SDL.h>

struct SdlTextureDeleter
{
    void operator()(SDL_Texture* texture) const noexcept
    {
        SDL_DestroyTexture(texture);
    }
};

using SdlTexture = std::unique_ptr<SDL_Texture, SdlTextureDeleter>;
