#pragma once

namespace vw {
class SDL_Initializer;
}
extern "C" {
vw::SDL_Initializer *vw_create_SDL_Initializer();
void vw_destroy_SDL_Initializer(vw::SDL_Initializer *);
}
