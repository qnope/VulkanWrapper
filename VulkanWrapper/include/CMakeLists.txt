target_sources(VulkanWrapper PRIVATE
    VulkanWrapper/3rd_party.h

    VulkanWrapper/fwd.h
    VulkanWrapper/Utils/Algos.h
    VulkanWrapper/Utils/exceptions.h
    VulkanWrapper/Utils/ObjectWithHandle.h

    VulkanWrapper/Window/SDL_Initializer.h
    VulkanWrapper/Window/Window.h

    VulkanWrapper/Vulkan/Instance.h
    VulkanWrapper/Vulkan/Device.h
    VulkanWrapper/Vulkan/Queue.h
    VulkanWrapper/Vulkan/Swapchain.h
    VulkanWrapper/Vulkan/PresentQueue.h
    VulkanWrapper/Vulkan/DeviceFinder.h
    VulkanWrapper/Vulkan/PhysicalDevice.h
    VulkanWrapper/Vulkan/Surface.h

    VulkanWrapper/Image/Image.h
    VulkanWrapper/Image/ImageView.h
    VulkanWrapper/Image/Framebuffer.h

    VulkanWrapper/Pipeline/ShaderModule.h
    VulkanWrapper/Pipeline/Pipeline.h
    VulkanWrapper/Pipeline/PipelineLayout.h

    VulkanWrapper/RenderPass/Attachment.h
    VulkanWrapper/RenderPass/RenderPass.h
    VulkanWrapper/RenderPass/Subpass.h

    VulkanWrapper/Command/CommandPool.h
    VulkanWrapper/Command/CommandBuffer.h

    VulkanWrapper/Synchronization/Fence.h
    VulkanWrapper/Synchronization/Semaphore.h
)
