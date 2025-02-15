#include "Window/Window.h"

#include <VulkanWrapper/Vulkan/Surface.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>
#include <VulkanWrapper/Window/Window.h>

vw::Window *vw_create_window(const VwWindowCreateArguments *arguments) {
    auto window = vw::WindowBuilder(*arguments->initializer)
                      .with_title(arguments->title)
                      .sized(arguments->width, arguments->height)
                      .build();
    return new vw::Window(std::move(window));
}

bool vw_is_close_window_requested(const vw::Window *window) {
    return window->is_close_requested();
}

const char *const *
vw_get_required_extensions_from_window(const vw::Window *window, int *number) {
    auto required = window->get_required_instance_extensions();
    *number = required.size();
    return required.data();
}

void vw_update_window(vw::Window *window) { window->update(); }

void vw_destroy_window(vw::Window *window) { delete window; }

vw::Surface *vw_create_surface_from_window(const vw::Window *window,
                                           const vw::Instance *instance) {
    return new vw::Surface{window->create_surface(*instance)};
}

void vw_destroy_surface(vw::Surface *surface) { delete surface; }

vw::Swapchain *vw_create_swapchain_from_window(const vw::Window *window,
                                               const vw::Device *device,
                                               const vw::Surface *surface) {
    return new vw::Swapchain{
        window->create_swapchain(*device, surface->handle())};
}
