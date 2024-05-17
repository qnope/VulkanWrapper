target_sources(3DRenderer PRIVATE
    3DRenderer/Core/Utils/exceptions.cpp
    3DRenderer/Core/Window/SDL_Initializer.cpp
    3DRenderer/Core/Window/Window.cpp

    3DRenderer/Core/Vulkan/Instance.cpp
    3DRenderer/Core/Vulkan/Device.cpp
    3DRenderer/Core/Vulkan/Queue.cpp
    3DRenderer/Core/Vulkan/PresentQueue.cpp
    3DRenderer/Core/Vulkan/DeviceFinder.cpp
    3DRenderer/Core/Vulkan/PhysicalDevice.cpp
)
