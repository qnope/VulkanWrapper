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

    bool is_close_requested() const noexcept;

    std::span<const char * const> get_required_instance_extensions() const noexcept;
    Surface create_surface(const Instance &instance) const;

    Swapchain create_swapchain(const Device &device,
                               vk::SurfaceKHR surface) const;

  private:
    Window(const SDL_Initializer &initializer, std::string_view name, int width,
           int height);

  private:
    const SDL_Initializer &m_initializer;
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    bool m_closeRequested = false;
    int m_width;
    int m_height;
};

class WindowBuilder {
  public:
    WindowBuilder(const SDL_Initializer &initializer);
    WindowBuilder(SDL_Initializer &&initializer) = delete;

    WindowBuilder &&with_title(std::string_view name) &&;
    WindowBuilder &&sized(int width, int height) &&;

    Window build() &&;

  private:
    const SDL_Initializer &initializer;
    std::string_view name = "3D Renderer";
    int width = 0;
    int height = 0;
};

} // namespace vw
