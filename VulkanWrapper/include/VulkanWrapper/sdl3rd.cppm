module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3_image/SDL_image.h>
#include <vulkan/vulkan.h>

export module sdl3rd;

// SDL3 types and functions
export using ::SDL_Window;
export using ::SDL_Event;
export using ::SDL_Surface;
export using ::SDL_PixelFormat;
export using ::SDL_InitSubSystem;
export using ::SDL_Quit;
export using ::SDL_CreateWindow;
export using ::SDL_DestroyWindow;
export using ::SDL_PollEvent;
export using ::SDL_GetError;
export using ::SDL_ConvertSurface;
export using ::SDL_CreateSurfaceFrom;
export using ::SDL_SaveBMP;
export using ::SDL_DestroySurface;
export using ::SDL_Vulkan_CreateSurface;
export using ::SDL_Vulkan_GetInstanceExtensions;
export using ::IMG_Load;
export using ::IMG_SavePNG;
export using ::IMG_SaveJPG;
export using ::SDL_PIXELFORMAT_ABGR8888;
export using ::SDL_EVENT_WINDOW_CLOSE_REQUESTED;
export using ::SDL_EVENT_WINDOW_RESIZED;

// SDL macro constants (macros cannot cross module boundaries)
export constexpr auto vw_SDL_INIT_VIDEO = SDL_INIT_VIDEO;
export constexpr auto vw_SDL_INIT_EVENTS = SDL_INIT_EVENTS;
export constexpr auto vw_SDL_WINDOW_VULKAN = SDL_WINDOW_VULKAN;
export constexpr auto vw_SDL_WINDOW_RESIZABLE = SDL_WINDOW_RESIZABLE;
