module vw.window;
import sdl3rd;
import vw.vulkan;
import vw.utils;

namespace vw {

SDL_Initializer::SDL_Initializer() {
    check_sdl(SDL_InitSubSystem(vw_SDL_INIT_EVENTS | vw_SDL_INIT_VIDEO),
              "Failed to initialize SDL subsystem");
}

SDL_Initializer::~SDL_Initializer() { SDL_Quit(); }

} // namespace vw
