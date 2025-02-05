#include "Window.h"

vw::Window *vw_create_Window(const vw::SDL_Initializer *initializer) {
    auto window = vw::WindowBuilder(*initializer)
                      .withTitle("Test FFI")
                      .sized(800, 600)
                      .build();
    return new vw::Window(std::move(window));
}

bool vw_close_Window_requested(const vw::Window *window) {
    return window->closeRequested();
}

ArrayConstString vw_required_extensions_from_window(const vw::Window *window) {
    auto required = window->requiredInstanceExtensions();
    return vw_create_array_const_string(required.data(), required.size());
}

void vw_update_Window(vw::Window *window) { window->update(); }

void vw_destroy_Window(vw::Window *window) { delete window; }
