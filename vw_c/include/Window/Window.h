#pragma once

#include "../Vulkan/Instance.h"
#include "SDL_Initializer.h"

namespace vw {
class Window;
class Surface;
class Swapchain;
class Device;
} // namespace vw

extern "C" {

struct VwWindowCreateArguments {
    const vw::SDL_Initializer *initializer;
    int width;
    int height;
    const char *title;
};

vw::Window *vw_create_window(const VwWindowCreateArguments *arguments);

bool vw_is_close_window_requested(const vw::Window *);
void vw_update_window(vw::Window *);

char const *const *
vw_get_required_extensions_from_window(const vw::Window *window, int *number);

vw::Surface *vw_create_surface_from_window(const vw::Window *window,
                                           const vw::Instance *instance);

vw::Swapchain *vw_create_swapchain_from_window(const vw::Window *window,
                                               const vw::Device *device,
                                               const vw::Surface *surface);

void vw_destroy_surface(vw::Surface *surface);

void vw_destroy_window(vw::Window *window);
}
