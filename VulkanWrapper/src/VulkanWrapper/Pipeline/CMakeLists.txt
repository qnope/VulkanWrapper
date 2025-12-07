target_sources(VulkanWrapperCoreLibrary PRIVATE
    ShaderModule.cpp
    Pipeline.cpp
    PipelineLayout.cpp
)

if(TARGET assimp::assimp)
    target_sources(VulkanWrapperCoreLibrary PRIVATE MeshRenderer.cpp)
endif()
