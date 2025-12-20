#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"

struct SDL_Window;

namespace vw {

class Window {
    struct WindowDeleter {
        void operator()(SDL_Window *window) const noexcept;
    };

    friend class WindowBuilder;

  public:
    void update() noexcept;

    [[nodiscard]] bool is_close_requested() const noexcept;
    [[nodiscard]] bool is_resized() const noexcept;
    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;

    [[nodiscard]] static std::span<const char *const>
    get_required_instance_extensions() noexcept;
    [[nodiscard]] Surface create_surface(const Instance &instance) const;

    [[nodiscard]] Swapchain
    create_swapchain(std::shared_ptr<const Device> device,
                     vk::SurfaceKHR surface) const;

  private:
    Window(std::shared_ptr<const SDL_Initializer> initializer,
           std::string_view name, Width width, Height height);

    std::shared_ptr<const SDL_Initializer> m_initializer;
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    bool m_closeRequested = false;
    bool m_resized = false;
    Width m_width;
    Height m_height;
};

class WindowBuilder {
  public:
    WindowBuilder(std::shared_ptr<const SDL_Initializer> initializer);

    WindowBuilder &with_title(std::string_view name);
    WindowBuilder &sized(Width width, Height height);

    Window build();

  private:
    std::shared_ptr<const SDL_Initializer> initializer;
    std::string_view name = "3D Renderer";
    Width width{};
    Height height{};
};

} // namespace vw
