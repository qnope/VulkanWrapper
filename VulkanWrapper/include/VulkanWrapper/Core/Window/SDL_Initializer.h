#pragma once

#include "VulkanWrapper/Core/Utils/exceptions.h"

namespace vw {
using InitializationException = TaggedException<struct InitializationTag>;

class SDL_Initializer {
  public:
    SDL_Initializer();
    ~SDL_Initializer();
};
} // namespace vw
