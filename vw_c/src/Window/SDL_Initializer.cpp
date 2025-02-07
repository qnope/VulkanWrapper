#include "Window/SDL_Initializer.h"

#include <VulkanWrapper/Window/SDL_Initializer.h>

vw::SDL_Initializer *vw_create_sdl_initializer() {
    return new vw::SDL_Initializer;
}

void vw_destroy_sdl_initializer(vw::SDL_Initializer *ptr) { delete ptr; }
