cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapper_c PRIVATE
    Vulkan/Instance.cpp
    Window/SDL_Initializer.cpp
    Window/Window.cpp
    utils/array.cpp
)
