target_sources(VulkanWrapper PRIVATE
    VulkanWrapper/Utils/exceptions.cpp
    VulkanWrapper/Window/SDL_Initializer.cpp
    VulkanWrapper/Window/Window.cpp

    VulkanWrapper/Vulkan/Instance.cpp
    VulkanWrapper/Vulkan/Device.cpp
    VulkanWrapper/Vulkan/Queue.cpp
    VulkanWrapper/Vulkan/Swapchain.cpp
    VulkanWrapper/Vulkan/PresentQueue.cpp
    VulkanWrapper/Vulkan/DeviceFinder.cpp
    VulkanWrapper/Vulkan/PhysicalDevice.cpp

    VulkanWrapper/Image/Image.cpp
    VulkanWrapper/Image/ImageView.cpp

    VulkanWrapper/Pipeline/ShaderModule.cpp
    VulkanWrapper/Pipeline/Pipeline.cpp
    VulkanWrapper/Pipeline/PipelineLayout.cpp
)
