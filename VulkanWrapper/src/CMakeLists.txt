target_sources(VulkanWrapperCoreLibrary PRIVATE
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
    VulkanWrapper/Vulkan/Surface.cpp

    VulkanWrapper/Image/Image.cpp
    VulkanWrapper/Image/ImageView.cpp
    VulkanWrapper/Image/Framebuffer.cpp
    VulkanWrapper/Image/ImageLoader.cpp
    VulkanWrapper/Image/Sampler.cpp
    VulkanWrapper/Image/CombinedImage.cpp
    VulkanWrapper/Image/Mipmap.cpp

    VulkanWrapper/Pipeline/ShaderModule.cpp
    VulkanWrapper/Pipeline/Pipeline.cpp
    VulkanWrapper/Pipeline/PipelineLayout.cpp
    VulkanWrapper/Pipeline/MeshRenderer.cpp

    VulkanWrapper/RenderPass/Attachment.cpp
    VulkanWrapper/RenderPass/RenderPass.cpp
    VulkanWrapper/RenderPass/Subpass.cpp

    VulkanWrapper/Command/CommandPool.cpp
    VulkanWrapper/Command/CommandBuffer.cpp

    VulkanWrapper/Synchronization/Fence.cpp
    VulkanWrapper/Synchronization/Semaphore.cpp

    VulkanWrapper/Memory/vma.cpp
    VulkanWrapper/Memory/Allocator.cpp
    VulkanWrapper/Memory/Buffer.cpp
    VulkanWrapper/Memory/StagingBufferManager.cpp
    VulkanWrapper/Memory/Barrier.cpp

    VulkanWrapper/Descriptors/DescriptorSetLayout.cpp
    VulkanWrapper/Descriptors/DescriptorPool.cpp
    VulkanWrapper/Descriptors/DescriptorAllocator.cpp

    VulkanWrapper/Model/Importer.cpp
    VulkanWrapper/Model/Mesh.cpp
    VulkanWrapper/Model/MeshManager.cpp
    VulkanWrapper/Model/Material/Material.cpp
    VulkanWrapper/Model/Material/MaterialManager.cpp
    VulkanWrapper/Model/Material/MaterialManagerMap.cpp
    VulkanWrapper/Model/Material/TexturedMaterialManager.cpp
    VulkanWrapper/Model/Material/ColoredMaterialManager.cpp

    VulkanWrapper/Model/Internal/MaterialInfo.cpp
    VulkanWrapper/Model/Internal/MeshInfo.cpp
)
