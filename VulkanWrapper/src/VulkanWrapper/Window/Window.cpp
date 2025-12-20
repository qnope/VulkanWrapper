#include "VulkanWrapper/Window/Window.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Surface.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace vw {
void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
};

Window::Window(std::shared_ptr<const SDL_Initializer> initializer, std::string_view name,
               Width width, Height height)
    : m_initializer{std::move(initializer)}
    , m_width{width}
    , m_height{height} {
    auto *window = check_sdl(
        SDL_CreateWindow(name.data(), int(width), int(height),
                         SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE),
        "Failed to create SDL window");

    m_window.reset(window);
}

Surface Window::create_surface(const Instance &instance) const {
    VkSurfaceKHR surface{};
    check_sdl(SDL_Vulkan_CreateSurface(m_window.get(), instance.handle(),
                                       nullptr, &surface),
              "Failed to create Vulkan surface");

    return Surface{vk::UniqueSurfaceKHR(surface, instance.handle())};
}

Swapchain Window::create_swapchain(std::shared_ptr<const Device> device,
                                   vk::SurfaceKHR surface) const {
    return SwapchainBuilder(std::move(device), surface, m_width, m_height).build();
}

bool Window::is_close_requested() const noexcept { return m_closeRequested; }

bool Window::is_resized() const noexcept { return m_resized; }

Width Window::width() const noexcept { return m_width; }

Height Window::height() const noexcept { return m_height; }

std::span<char const *const>
Window::get_required_instance_extensions() noexcept {
    unsigned count{};
    const auto *array = SDL_Vulkan_GetInstanceExtensions(&count);
    return {array, count};
}

void Window::update() noexcept {
    m_resized = false;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            m_closeRequested = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            m_width = Width(event.window.data1);
            m_height = Height(event.window.data2);
            m_resized = true;
            break;
        default:
            break;
        }
    }
}

WindowBuilder::WindowBuilder(std::shared_ptr<const SDL_Initializer> initializer)
    : initializer{std::move(initializer)} {}

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
    return Window{std::move(initializer), name, width, height};
}

} // namespace vw
