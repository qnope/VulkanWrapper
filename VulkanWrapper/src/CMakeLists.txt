target_sources(VulkanWrapper PRIVATE
    VulkanWrapper/Core/Utils/exceptions.cpp
    VulkanWrapper/Core/Window/SDL_Initializer.cpp
    VulkanWrapper/Core/Window/Window.cpp

    VulkanWrapper/Core/Vulkan/Instance.cpp
    VulkanWrapper/Core/Vulkan/Device.cpp
    VulkanWrapper/Core/Vulkan/Queue.cpp
    VulkanWrapper/Core/Vulkan/Swapchain.cpp
    VulkanWrapper/Core/Vulkan/PresentQueue.cpp
    VulkanWrapper/Core/Vulkan/DeviceFinder.cpp
    VulkanWrapper/Core/Vulkan/PhysicalDevice.cpp
    VulkanWrapper/Core/Vulkan/Image.cpp
    VulkanWrapper/Core/Vulkan/ImageView.cpp

    VulkanWrapper/Core/Shaders/ShaderModule.cpp
)
