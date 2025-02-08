#pragma once

#include "../utils/array.h"
#include "../Vulkan/Instance.h"
#include "SDL_Initializer.h"

namespace vw {
class Window;
class Surface;
class Swapchain;
class Device;
} // namespace vw

extern "C" {

vw::Window *vw_create_window(const vw::SDL_Initializer *initializer, int width,
                             int height, const char *title);

bool vw_is_close_window_requested(const vw::Window *);
void vw_update_window(vw::Window *);

vw_ArrayConstString
vw_get_required_extensions_from_window(const vw::Window *window);

vw::Surface *vw_create_surface_from_window(const vw::Window *window,
                                           const vw::Instance *instance);

vw::Swapchain *vw_create_swapchain_from_window(const vw::Window *window,
                                               const vw::Device *device,
                                               const vw::Surface *surface);

void vw_destroy_swapchain(vw::Swapchain *swapchain);

void vw_destroy_surface(vw::Surface *surface);

void vw_destroy_window(vw::Window *window);
}
