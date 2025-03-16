#pragma once

#include "VulkanWrapper/Utils/exceptions.h"

namespace vw {
using InitializationException = TaggedException<struct InitializationTag>;

class SDL_Initializer {
  public:
    SDL_Initializer();
    SDL_Initializer(const SDL_Initializer &) = delete;
    SDL_Initializer(SDL_Initializer &&) = delete;
    SDL_Initializer &operator=(SDL_Initializer &&) = delete;
    SDL_Initializer &operator=(const SDL_Initializer &) = delete;
    ~SDL_Initializer();
};
} // namespace vw
