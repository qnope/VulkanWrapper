#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"

struct SDL_Window;

namespace vw {
using WindowInitializationException =
    TaggedException<struct WindowInitializationTag>;

using SurfaceCreationException = TaggedException<struct SurfaceCreationTag>;

class Window {
    struct WindowDeleter {
        void operator()(SDL_Window *window) const noexcept;
    };

    friend class WindowBuilder;

  public:
    void update() noexcept;

    bool closeRequested() const noexcept;

    std::vector<const char *> requiredInstanceExtensions() const noexcept;
    vk::UniqueSurfaceKHR createSurface(const Instance &instance) const;

    SwapchainBuilder createSwapchain(Device &device, vk::SurfaceKHR surface);

  private:
    Window(SDL_Initializer &initializer, std::string_view name, int width,
           int height);

  private:
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    bool m_closeRequested = false;
    int m_width;
    int m_height;
};

class WindowBuilder {
  public:
    WindowBuilder(SDL_Initializer &initializer);

    WindowBuilder &&withTitle(std::string_view name) &&;
    WindowBuilder &&sized(int width, int height) &&;

    Window build() &&;

  private:
    SDL_Initializer &initializer;
    std::string_view name = "3D Renderer";
    int width = 0;
    int height = 0;
};

} // namespace vw
