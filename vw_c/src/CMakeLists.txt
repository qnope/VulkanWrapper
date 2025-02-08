cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapper_c PRIVATE
    Window/SDL_Initializer.cpp
    Window/Window.cpp

    utils/array.cpp

    Vulkan/Device.cpp
    Vulkan/Instance.cpp

    Pipeline/ShaderModule.cpp
)
