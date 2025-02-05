#pragma once

#include "array.h"
#include "SDL_Initializer.h"

namespace vw {
class Window;
}

extern "C" {

vw::Window *vw_create_Window(const vw::SDL_Initializer *initializer);

bool vw_close_Window_requested(const vw::Window *);
void vw_update_Window(vw::Window *);

ArrayConstString vw_required_extensions_from_window(const vw::Window *window);

void vw_destroy_Window(vw::Window *);
}
