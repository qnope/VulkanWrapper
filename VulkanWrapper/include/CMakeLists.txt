target_sources(VulkanWrapper PRIVATE
    VulkanWrapper/3rd_party.h

    VulkanWrapper/Core/fwd.h
    VulkanWrapper/Core/Utils/Algos.h
    VulkanWrapper/Core/Utils/exceptions.h

    VulkanWrapper/Core/Window/SDL_Initializer.h
    VulkanWrapper/Core/Window/Window.h

    VulkanWrapper/Core/Vulkan/ObjectWithHandle.h
    VulkanWrapper/Core/Vulkan/Instance.h
    VulkanWrapper/Core/Vulkan/Device.h
    VulkanWrapper/Core/Vulkan/Queue.h
    VulkanWrapper/Core/Vulkan/Swapchain.h
    VulkanWrapper/Core/Vulkan/PresentQueue.h
    VulkanWrapper/Core/Vulkan/DeviceFinder.h
    VulkanWrapper/Core/Vulkan/PhysicalDevice.h
    VulkanWrapper/Core/Vulkan/Image.h
)
