#include <SDL3/SDL.h>
#include <VulkanWrapper/Utils/exceptions.h>
#include <VulkanWrapper/Window/SDL_Initializer.h>

namespace vw {

SDL_Initializer::SDL_Initializer() {
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        throw InitializationException{std::source_location::current()};
    }
}

SDL_Initializer::~SDL_Initializer() { SDL_Quit(); }

} // namespace vw
