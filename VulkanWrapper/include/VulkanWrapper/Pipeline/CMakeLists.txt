target_sources(VulkanWrapperCoreLibrary PUBLIC
    ShaderModule.h
    Pipeline.h
    PipelineLayout.h
)

if(TARGET assimp::assimp)
    target_sources(VulkanWrapperCoreLibrary PUBLIC MeshRenderer.h)
endif()
