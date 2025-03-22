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

    [[nodiscard]] bool is_close_requested() const noexcept;

    [[nodiscard]] static std::span<const char *const>
    get_required_instance_extensions() noexcept;
    [[nodiscard]] Surface create_surface(const Instance &instance) const;

    [[nodiscard]] Swapchain create_swapchain(const Device &device,
                                             vk::SurfaceKHR surface) const;

  private:
    Window(const SDL_Initializer &initializer, std::string_view name,
           Width width, Height height);

    const SDL_Initializer *m_initializer;
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    bool m_closeRequested = false;
    Width m_width;
    Height m_height;
};

class WindowBuilder {
  public:
    WindowBuilder(const SDL_Initializer &initializer);
    WindowBuilder(SDL_Initializer &&initializer) = delete;

    WindowBuilder &&with_title(std::string_view name) &&;
    WindowBuilder &&sized(Width width, Height height) &&;

    Window build() &&;

  private:
    const SDL_Initializer *initializer;
    std::string_view name = "3D Renderer";
    Width width{};
    Height height{};
};

} // namespace vw
