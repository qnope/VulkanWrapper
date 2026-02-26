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

# Header unit: compiled ONCE, then imported by all module interface and
# implementation units.  Eliminates N×(vulkan.hpp + vk_mem_alloc.h + glm +
# std-headers) cold parses – previously one full parse per translation unit.
target_sources(VulkanWrapperCoreLibrary
    PUBLIC FILE_SET CXX_MODULE_HEADER_UNITS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES include/VulkanWrapper/vw_vulkan.h
)

# Module interface units (partitions + primary)
target_sources(VulkanWrapperCoreLibrary
    PUBLIC FILE_SET CXX_MODULES FILES
        modules/vw.cppm
        modules/vw-core.cppm
        modules/vw-utils.cppm
        modules/vw-vulkan.cppm
        modules/vw-command.cppm
        modules/vw-synchronization.cppm
        modules/vw-memory.cppm
        modules/vw-image.cppm
        modules/vw-descriptors.cppm
        modules/vw-pipeline.cppm
        modules/vw-shader.cppm
        modules/vw-window.cppm
        modules/vw-renderpass.cppm
        modules/vw-model.cppm
        modules/vw-raytracing.cppm
        modules/vw-random.cppm
)

# Implementation units (module impl .cpp files + non-module vma.cpp)
target_sources(VulkanWrapperCoreLibrary PRIVATE
    # Utils
    src/VulkanWrapper/Utils/Error.cpp
    # Vulkan
    # DispatchLoaderStorage is a plain (non-module) TU: the sole file that
    # must include vulkan.hpp directly to instantiate the dynamic dispatcher.
    src/VulkanWrapper/Vulkan/DispatchLoaderStorage.cpp
    src/VulkanWrapper/Vulkan/Instance.cpp
    src/VulkanWrapper/Vulkan/Device.cpp
    src/VulkanWrapper/Vulkan/Queue.cpp
    src/VulkanWrapper/Vulkan/Swapchain.cpp
    src/VulkanWrapper/Vulkan/PresentQueue.cpp
    src/VulkanWrapper/Vulkan/DeviceFinder.cpp
    src/VulkanWrapper/Vulkan/PhysicalDevice.cpp
    src/VulkanWrapper/Vulkan/Surface.cpp
    # Command
    src/VulkanWrapper/Command/CommandPool.cpp
    src/VulkanWrapper/Command/CommandBuffer.cpp
    # Synchronization
    src/VulkanWrapper/Synchronization/Fence.cpp
    src/VulkanWrapper/Synchronization/Semaphore.cpp
    src/VulkanWrapper/Synchronization/ResourceTracker.cpp
    # Memory (vma.cpp is non-module TU)
    src/VulkanWrapper/Memory/vma.cpp
    src/VulkanWrapper/Memory/Allocator.cpp
    src/VulkanWrapper/Memory/Buffer.cpp
    src/VulkanWrapper/Memory/StagingBufferManager.cpp
    src/VulkanWrapper/Memory/Barrier.cpp
    src/VulkanWrapper/Memory/Interval.cpp
    src/VulkanWrapper/Memory/IntervalSet.cpp
    src/VulkanWrapper/Memory/Transfer.cpp
    src/VulkanWrapper/Memory/UniformBufferAllocator.cpp
    # Image
    src/VulkanWrapper/Image/Image.cpp
    src/VulkanWrapper/Image/ImageView.cpp
    src/VulkanWrapper/Image/ImageLoader.cpp
    src/VulkanWrapper/Image/Sampler.cpp
    src/VulkanWrapper/Image/CombinedImage.cpp
    src/VulkanWrapper/Image/Mipmap.cpp
    # Descriptors
    src/VulkanWrapper/Descriptors/DescriptorSetLayout.cpp
    src/VulkanWrapper/Descriptors/DescriptorPool.cpp
    src/VulkanWrapper/Descriptors/DescriptorAllocator.cpp
    src/VulkanWrapper/Descriptors/DescriptorSet.cpp
    # Pipeline
    src/VulkanWrapper/Pipeline/ShaderModule.cpp
    src/VulkanWrapper/Pipeline/Pipeline.cpp
    src/VulkanWrapper/Pipeline/ComputePipeline.cpp
    src/VulkanWrapper/Pipeline/PipelineLayout.cpp
    src/VulkanWrapper/Pipeline/MeshRenderer.cpp
    # Shader
    src/VulkanWrapper/Shader/ShaderCompiler.cpp
    # Window
    src/VulkanWrapper/Window/SDL_Initializer.cpp
    src/VulkanWrapper/Window/Window.cpp
    # RenderPass
    src/VulkanWrapper/RenderPass/IndirectLightPass.cpp
    src/VulkanWrapper/RenderPass/ScreenSpacePass.cpp
    src/VulkanWrapper/RenderPass/SkyParameters.cpp
    src/VulkanWrapper/RenderPass/SkyPass.cpp
    src/VulkanWrapper/RenderPass/ToneMappingPass.cpp
    # Model
    src/VulkanWrapper/Model/Importer.cpp
    src/VulkanWrapper/Model/Mesh.cpp
    src/VulkanWrapper/Model/MeshManager.cpp
    src/VulkanWrapper/Model/Internal/MaterialInfo.cpp
    src/VulkanWrapper/Model/Internal/MeshInfo.cpp
    src/VulkanWrapper/Model/Material/Material.cpp
    src/VulkanWrapper/Model/Material/BindlessTextureManager.cpp
    src/VulkanWrapper/Model/Material/BindlessMaterialManager.cpp
    src/VulkanWrapper/Model/Material/TexturedMaterialHandler.cpp
    src/VulkanWrapper/Model/Material/EmissiveTexturedMaterialHandler.cpp
    src/VulkanWrapper/Model/Material/ColoredMaterialHandler.cpp
    # RayTracing
    src/VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.cpp
    src/VulkanWrapper/RayTracing/TopLevelAccelerationStructure.cpp
    src/VulkanWrapper/RayTracing/RayTracingPipeline.cpp
    src/VulkanWrapper/RayTracing/RayTracedScene.cpp
    src/VulkanWrapper/RayTracing/ShaderBindingTable.cpp
    # Random
    src/VulkanWrapper/Random/RandomSamplingBuffer.cpp
    src/VulkanWrapper/Random/NoiseTexture.cpp
)

# Keep include dir for 3rd_party.h (used in global module fragments)
target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)
target_compile_definitions(VulkanWrapperCoreLibrary PRIVATE VW_LIB)
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

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)
