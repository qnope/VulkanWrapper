#include "VulkanWrapper/Window/Window.h"

#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Surface.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace vw {
void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
};

Window::Window(const SDL_Initializer &initializer, std::string_view name,
               Width width, Height height)
    : m_initializer{&initializer}
    , m_width{width}
    , m_height{height} {
    auto *window = SDL_CreateWindow(name.begin(), int(width), int(height),
                                    SDL_WINDOW_VULKAN);

    if (window == nullptr) {
        throw WindowInitializationException{std::source_location::current()};
    }

    m_window.reset(window);
}

Surface Window::create_surface(const Instance &instance) const {
    VkSurfaceKHR surface{};
    auto x = SDL_Vulkan_CreateSurface(m_window.get(), instance.handle(),
                                      nullptr, &surface);

    if (!x)
        throw SurfaceCreationException{std::source_location::current()};

    return Surface{vk::UniqueSurfaceKHR(surface, instance.handle())};
}

Swapchain Window::create_swapchain(const Device &device,
                                   vk::SurfaceKHR surface) const {
    return SwapchainBuilder(device, surface, m_width, m_height).build();
}

bool Window::is_close_requested() const noexcept { return m_closeRequested; }

std::span<char const *const>
Window::get_required_instance_extensions() noexcept {
    unsigned count{};
    const auto *array = SDL_Vulkan_GetInstanceExtensions(&count);
    return {array, count};
}

void Window::update() noexcept {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            m_closeRequested = true;
        default:
            break;
        }
    }
}

WindowBuilder::WindowBuilder(const SDL_Initializer &initializer)
    : initializer{&initializer} {}

WindowBuilder &&WindowBuilder::with_title(std::string_view name) && {
    this->name = name;
    return std::move(*this);
}

WindowBuilder &&WindowBuilder::sized(Width width, Height height) && {
    this->width = width;
    this->height = height;
    return std::move(*this);
}

Window WindowBuilder::build() && {
    return Window{*initializer, name, width, height};
}

} // namespace vw
