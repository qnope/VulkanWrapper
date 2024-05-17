#include "3DRenderer/Core/Window/Window.h"
#include "3DRenderer/Core/Utils/exceptions.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "3DRenderer/Core/Vulkan/Instance.h"

namespace r3d {
void Window::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
};

Window::Window(SDL_Initializer &initializer, std::string_view name, int width,
               int height) {
    auto window = SDL_CreateWindow(name.begin(), SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, width, height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    if (!window) {
        throw WindowInitializationException{std::source_location::current()};
    }

    m_window.reset(window);
}

vk::SurfaceKHR Window::createSurface(const Instance &instance) {
    VkSurfaceKHR surface;
    auto x =
        SDL_Vulkan_CreateSurface(m_window.get(), instance.handle(), &surface);

    auto d = SDL_GetError();
    return surface;
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

} // namespace r3d
