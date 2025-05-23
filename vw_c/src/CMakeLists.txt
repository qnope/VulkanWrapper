cmake_minimum_required(VERSION 3.14)

target_sources(VulkanWrapperWrapperLibrary PRIVATE
    Window/SDL_Initializer.cpp
    Window/Window.cpp

    Vulkan/Device.cpp
    Vulkan/Instance.cpp
    Vulkan/Swapchain.cpp
    Vulkan/PresentQueue.cpp
    Vulkan/Queue.cpp

    Pipeline/ShaderModule.cpp
    Pipeline/PipelineLayout.cpp
    Pipeline/Pipeline.cpp

    RenderPass/Attachment.cpp
    RenderPass/RenderPass.cpp
    RenderPass/Subpass.cpp

    Synchronization/Fence.cpp
    Synchronization/Semaphore.cpp

    Command/CommandPool.cpp
    Command/CommandBuffer.cpp

    Image/Framebuffer.cpp
    Image/ImageView.cpp
    Image/Image.cpp
)
