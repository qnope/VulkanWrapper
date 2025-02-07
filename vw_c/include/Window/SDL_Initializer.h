#pragma once

namespace vw {
class SDL_Initializer;
}
extern "C" {
vw::SDL_Initializer *vw_create_sdl_initializer();
void vw_destroy_sdl_initializer(vw::SDL_Initializer *);
}
