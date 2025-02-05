#include "SDL_Initializer.h"

vw::SDL_Initializer *vw_create_SDL_Initializer() {
    return new vw::SDL_Initializer;
}

void vw_destroy_SDL_Initializer(vw::SDL_Initializer *ptr) { delete ptr; }
