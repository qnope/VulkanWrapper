cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapper_c PRIVATE
    Window/SDL_Initializer.h
    Window/Window.h

    utils/array.h
    bindings.h

    Vulkan/Device.h
    Vulkan/Instance.h

    Pipeline/ShaderModule.h
)
