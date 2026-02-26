module;
#include "VulkanWrapper/3rd_party.h"
#include <SDL3/SDL.h>
module vw;
import "VulkanWrapper/vw_vulkan.h";
namespace vw {

SDL_Initializer::SDL_Initializer() {
    check_sdl(SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO),
              "Failed to initialize SDL subsystem");
}

SDL_Initializer::~SDL_Initializer() { SDL_Quit(); }

} // namespace vw
