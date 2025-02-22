cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapper_c PRIVATE
    Window/SDL_Initializer.h
    Window/Window.h

    bindings.h

    Vulkan/enums.h
    Vulkan/Device.h
    Vulkan/Instance.h
    Vulkan/Swapchain.h
    Vulkan/PresentQueue.h
    Vulkan/Queue.h

    Pipeline/ShaderModule.h
    Pipeline/PipelineLayout.h
    Pipeline/Pipeline.h

    RenderPass/Attachment.h
    RenderPass/RenderPass.h
    RenderPass/Subpass.h

    Synchronization/Semaphore.h
    Synchronization/Fence.h

    Command/CommandPool.h

    Image/Framebuffer.h
    Image/ImageView.h
    Image/Image.h

    utils/utils.h
)
