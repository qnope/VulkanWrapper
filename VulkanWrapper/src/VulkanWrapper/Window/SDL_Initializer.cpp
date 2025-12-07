#include <SDL3/SDL.h>
#include <VulkanWrapper/Utils/Error.h>
#include <VulkanWrapper/Window/SDL_Initializer.h>

namespace vw {

SDL_Initializer::SDL_Initializer() {
    check_sdl(SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO),
              "Failed to initialize SDL subsystem");
}

SDL_Initializer::~SDL_Initializer() { SDL_Quit(); }

} // namespace vw
