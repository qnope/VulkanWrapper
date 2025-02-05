cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapper_c PRIVATE
    SDL_Initializer.cpp
    Window.cpp
    array.cpp
)
