#pragma once

#include <VulkanWrapper/Window/SDL_Initializer.h>

extern "C" {
vw::SDL_Initializer *vw_create_SDL_Initializer();
void vw_destroy_SDL_Initializer(vw::SDL_Initializer *);
}
