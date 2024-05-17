#pragma once

#include "3DRenderer/3rd_party.h"
#include "3DRenderer/Core/fwd.h"
#include <memory>
#include <string_view>

struct SDL_Window;

namespace r3d {
class Window {
    struct WindowDeleter {
        void operator()(SDL_Window *window) const noexcept;
    };

    friend class WindowBuilder;

  public:
    void update() noexcept;

    bool closeRequested() const noexcept;

    std::vector<const char *> requiredInstanceExtensions() const noexcept;
    vk::SurfaceKHR createSurface(const Instance &instance);

  private:
    Window(SDL_Initializer &initializer, std::string_view name, int width,
           int height);

  private:
    std::unique_ptr<SDL_Window, WindowDeleter> m_window;
    bool m_closeRequested = false;
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

} // namespace r3d
