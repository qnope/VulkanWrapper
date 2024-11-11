#include "VulkanWrapper/Core/Window/Window.h"

#include "VulkanWrapper/Core/Utils/exceptions.h"
#include "VulkanWrapper/Core/Vulkan/Instance.h"
#include "VulkanWrapper/Core/Vulkan/Swapchain.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

namespace vw {
void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
};

Window::Window(SDL_Initializer &initializer, std::string_view name, int width,
               int height)
    : m_width{width}
    , m_height{height} {
    auto window = SDL_CreateWindow(name.begin(), SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, width, height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    if (!window) {
        throw WindowInitializationException{std::source_location::current()};
    }

    m_window.reset(window);
}

vk::UniqueSurfaceKHR Window::createSurface(const Instance &instance) const {
    VkSurfaceKHR surface;
    SDL_bool x =
        SDL_Vulkan_CreateSurface(m_window.get(), instance.handle(), &surface);

    if (x == SDL_FALSE)
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
    SDL_Vulkan_GetInstanceExtensions(m_window.get(), &count, nullptr);
    std::vector<const char *> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(m_window.get(), &count, extensions.data());
    return extensions;
}

void Window::update() noexcept {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE)
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
