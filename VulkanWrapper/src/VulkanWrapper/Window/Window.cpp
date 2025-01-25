#include "VulkanWrapper/Window/Window.h"

#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/Instance.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace vw {
void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
};

Window::Window(SDL_Initializer &initializer, std::string_view name, int width,
               int height)
    : m_width{width}
    , m_height{height} {
    auto window =
        SDL_CreateWindow(name.begin(), width, height, SDL_WINDOW_VULKAN);

    if (!window) {
        throw WindowInitializationException{std::source_location::current()};
    }

    m_window.reset(window);
}

vk::UniqueSurfaceKHR Window::createSurface(const Instance &instance) const {
    VkSurfaceKHR surface;
    auto x = SDL_Vulkan_CreateSurface(m_window.get(), instance.handle(),
                                      nullptr, &surface);

    if (x == false)
        throw SurfaceCreationException{std::source_location::current()};

    return vk::UniqueSurfaceKHR(surface, instance.handle());
}

SwapchainBuilder Window::createSwapchain(Device &device,
                                         vk::SurfaceKHR surface) {
    return SwapchainBuilder(device, surface, m_width, m_height);
}

bool Window::closeRequested() const noexcept { return m_closeRequested; }

std::vector<const char *> Window::requiredInstanceExtensions() const noexcept {
    unsigned count;
    auto array = SDL_Vulkan_GetInstanceExtensions(&count);
    std::vector<const char *> extensions(array, array + count);
    return extensions;
}

void Window::update() noexcept {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            m_closeRequested = true;
        }
    }
}

WindowBuilder::WindowBuilder(SDL_Initializer &initializer)
    : initializer{initializer} {}

WindowBuilder &&WindowBuilder::withTitle(std::string_view name) && {
    this->name = name;
    return std::move(*this);
}

WindowBuilder &&WindowBuilder::sized(int width, int height) && {
    this->width = width;
    this->height = height;
    return std::move(*this);
}

Window WindowBuilder::build() && {
    return Window{initializer, name, width, height};
}

} // namespace vw
