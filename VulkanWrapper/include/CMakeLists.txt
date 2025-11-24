target_sources(VulkanWrapperCoreLibrary PRIVATE
    VulkanWrapper/3rd_party.h

    VulkanWrapper/fwd.h
    VulkanWrapper/Utils/Algos.h
    VulkanWrapper/Utils/exceptions.h
    VulkanWrapper/Utils/ObjectWithHandle.h
    VulkanWrapper/Utils/IdentifierTag.h
    VulkanWrapper/Utils/Alignment.h

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

    VulkanWrapper/Image/ImageLoader.h
    VulkanWrapper/Image/Sampler.h
    VulkanWrapper/Image/CombinedImage.h
    VulkanWrapper/Image/Mipmap.h

    VulkanWrapper/Pipeline/ShaderModule.h
    VulkanWrapper/Pipeline/Pipeline.h
    VulkanWrapper/Pipeline/PipelineLayout.h
    VulkanWrapper/Pipeline/MeshRenderer.h


    VulkanWrapper/RenderPass/Subpass.h

    VulkanWrapper/Command/CommandPool.h
    VulkanWrapper/Command/CommandBuffer.h

    VulkanWrapper/Synchronization/Fence.h
    VulkanWrapper/Synchronization/Semaphore.h

    VulkanWrapper/Descriptors/Vertex.h
    VulkanWrapper/Descriptors/DescriptorSetLayout.h
    VulkanWrapper/Descriptors/DescriptorPool.h
    VulkanWrapper/Descriptors/DescriptorAllocator.h

    VulkanWrapper/Memory/Allocator.h
    VulkanWrapper/Memory/Buffer.h
    VulkanWrapper/Memory/StagingBufferManager.h
    VulkanWrapper/Memory/Barrier.h
    VulkanWrapper/Memory/BufferList.h

    VulkanWrapper/Model/Importer.h
    VulkanWrapper/Model/Mesh.h
    VulkanWrapper/Model/MeshManager.h

    VulkanWrapper/Model/Material/Material.h
    VulkanWrapper/Model/Material/MaterialTypeTag.h
    VulkanWrapper/Model/Material/MaterialManager.h
    VulkanWrapper/Model/Material/MaterialManagerMap.h
    VulkanWrapper/Model/Material/TexturedMaterialManager.h
    VulkanWrapper/Model/Material/ColoredMaterialManager.h
    VulkanWrapper/Model/Material/MaterialFactory.h
    VulkanWrapper/Model/Material/MaterialPriority.h

    VulkanWrapper/Model/Internal/MaterialInfo.h
    VulkanWrapper/Model/Internal/MeshInfo.h
)

add_subdirectory(VulkanWrapper)
