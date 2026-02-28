cmake_minimum_required(VERSION 3.28)

include(FetchContent)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED)

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)
target_compile_definitions(VulkanWrapperCoreLibrary
    PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_EXCEPTIONS
        "VULKAN_HPP_ASSERT_ON_RESULT="
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
    INTERFACE
        "VULKAN_HPP_DEFAULT_DISPATCHER=vw::DefaultDispatcher()"
    PRIVATE
        VW_LIB
)
target_link_libraries(VulkanWrapperCoreLibrary
        PUBLIC
        glm::glm
        assimp::assimp
        SDL3::SDL3
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        SDL3_image::SDL3_image
        PRIVATE
        $<$<BOOL:${ENABLE_COVERAGE}>:coverage_options>
        unofficial::shaderc::shaderc
)

# Add tests subdirectory
add_subdirectory(tests)
target_link_libraries(VulkanWrapperCoreLibrary PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)
target_sources(VulkanWrapperCoreLibrary
    PUBLIC
    FILE_SET CXX_MODULES FILES
        include/VulkanWrapper/std3rd.cppm
        include/VulkanWrapper/vulkan3rd.cppm
        include/VulkanWrapper/vma3rd.cppm
        include/VulkanWrapper/glm3rd.cppm
        include/VulkanWrapper/assimp3rd.cppm
        include/VulkanWrapper/sdl3rd.cppm
        include/VulkanWrapper/shaderc3rd.cppm
        src/VulkanWrapper/vw.cppm
        src/VulkanWrapper/Utils/Alignment.cppm
        src/VulkanWrapper/Utils/Algos.cppm
        src/VulkanWrapper/Utils/Error.cppm
        src/VulkanWrapper/Utils/ObjectWithHandle.cppm
        src/VulkanWrapper/Utils/Utils.cppm
        src/VulkanWrapper/Core/PhysicalDevice.cppm
        src/VulkanWrapper/Core/PresentQueue.cppm
        src/VulkanWrapper/Core/Instance.cppm
        src/VulkanWrapper/Core/Surface.cppm
        src/VulkanWrapper/Core/Device.cppm
        src/VulkanWrapper/Core/DeviceFinder.cppm
        src/VulkanWrapper/Core/Queue.cppm
        src/VulkanWrapper/Core/Swapchain.cppm
        src/VulkanWrapper/Core/Vulkan.cppm
        src/VulkanWrapper/Core/Sync.cppm
        src/VulkanWrapper/Core/Command.cppm
        src/VulkanWrapper/Core/Memory.cppm
        src/VulkanWrapper/Core/CommandPool.cppm
        src/VulkanWrapper/Core/CommandBuffer.cppm
        src/VulkanWrapper/Core/Fence.cppm
        src/VulkanWrapper/Core/Semaphore.cppm
        src/VulkanWrapper/Core/ResourceTracker.cppm
        src/VulkanWrapper/Core/Interval.cppm
        src/VulkanWrapper/Core/IntervalSet.cppm
        src/VulkanWrapper/Core/BufferUsage.cppm
        src/VulkanWrapper/Core/Buffer.cppm
        src/VulkanWrapper/Core/Allocator.cppm
        src/VulkanWrapper/Core/AllocateBufferUtils.cppm
        src/VulkanWrapper/Core/BufferList.cppm
        src/VulkanWrapper/Core/Barrier.cppm
        src/VulkanWrapper/Core/StagingBufferManager.cppm
        src/VulkanWrapper/Core/Transfer.cppm
        src/VulkanWrapper/Core/UniformBufferAllocator.cppm
        src/VulkanWrapper/Core/Image.cppm
        src/VulkanWrapper/Core/ImageView.cppm
        src/VulkanWrapper/Core/Sampler.cppm
        src/VulkanWrapper/Core/CombinedImage.cppm
        src/VulkanWrapper/Core/Mipmap.cppm
        src/VulkanWrapper/Core/ImageLoader.cppm
        src/VulkanWrapper/Descriptors/Vertex.cppm
        src/VulkanWrapper/Descriptors/DescriptorSetLayout.cppm
        src/VulkanWrapper/Descriptors/DescriptorAllocator.cppm
        src/VulkanWrapper/Descriptors/DescriptorSet.cppm
        src/VulkanWrapper/Descriptors/DescriptorPool.cppm
        src/VulkanWrapper/Descriptors/Descriptors.cppm
        src/VulkanWrapper/Pipeline/ShaderModule.cppm
        src/VulkanWrapper/Pipeline/PipelineLayout.cppm
        src/VulkanWrapper/Pipeline/Pipeline.cppm
        src/VulkanWrapper/Pipeline/ComputePipeline.cppm
        src/VulkanWrapper/Pipeline/Pipeline_.cppm
        src/VulkanWrapper/Shader/ShaderCompiler.cppm
        src/VulkanWrapper/Shader/Shader.cppm
        src/VulkanWrapper/Window/SDL_Initializer.cppm
        src/VulkanWrapper/Window/Window.cppm
        src/VulkanWrapper/Window/Window_.cppm
        src/VulkanWrapper/Model/Material/MaterialTypeTag.cppm
        src/VulkanWrapper/Model/Material/MaterialPriority.cppm
        src/VulkanWrapper/Model/Material/MaterialData.cppm
        src/VulkanWrapper/Model/Material/Material.cppm
        src/VulkanWrapper/Model/Material/IMaterialTypeHandler.cppm
        src/VulkanWrapper/Model/Material/MaterialTypeHandler.cppm
        src/VulkanWrapper/Model/Material/BindlessTextureManager.cppm
        src/VulkanWrapper/Model/Material/BindlessMaterialManager.cppm
        src/VulkanWrapper/Model/Material/TexturedMaterialHandler.cppm
        src/VulkanWrapper/Model/Material/EmissiveTexturedMaterialHandler.cppm
        src/VulkanWrapper/Model/Material/ColoredMaterialHandler.cppm
        src/VulkanWrapper/Model/Internal/MaterialInfo.cppm
        src/VulkanWrapper/Model/Internal/MeshInfo.cppm
        src/VulkanWrapper/Model/Mesh.cppm
        src/VulkanWrapper/Model/Scene.cppm
        src/VulkanWrapper/Model/MeshManager.cppm
        src/VulkanWrapper/Model/Importer.cppm
        src/VulkanWrapper/Model/MeshRenderer.cppm
        src/VulkanWrapper/Model/Model.cppm
        src/VulkanWrapper/RayTracing/GeometryReference.cppm
        src/VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.cppm
        src/VulkanWrapper/RayTracing/TopLevelAccelerationStructure.cppm
        src/VulkanWrapper/RayTracing/RayTracingPipeline.cppm
        src/VulkanWrapper/RayTracing/ShaderBindingTable.cppm
        src/VulkanWrapper/RayTracing/RayTracedScene.cppm
        src/VulkanWrapper/RayTracing/RayTracing.cppm
        src/VulkanWrapper/RenderPass/SkyParameters.cppm
        src/VulkanWrapper/RenderPass/Subpass.cppm
        src/VulkanWrapper/RenderPass/ScreenSpacePass.cppm
        src/VulkanWrapper/RenderPass/SkyPass.cppm
        src/VulkanWrapper/RenderPass/ToneMappingPass.cppm
        src/VulkanWrapper/RenderPass/IndirectLightPass.cppm
        src/VulkanWrapper/RenderPass/RenderPass.cppm
        src/VulkanWrapper/Random/NoiseTexture.cppm
        src/VulkanWrapper/Random/RandomSamplingBuffer.cppm
        src/VulkanWrapper/Random/Random.cppm
)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)


