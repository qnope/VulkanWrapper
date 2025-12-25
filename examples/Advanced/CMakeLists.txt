
add_executable(Main main.cpp
    RenderPassInformation.h
    ZPass.h
    ColorPass.h
    SkyPass.h
    SunLightPass.h
    AmbientOcclusionPass.h
    DeferredRenderingManager.h
)

target_link_libraries(Main PRIVATE App)
target_precompile_headers(Main REUSE_FROM VulkanWrapperCoreLibrary)

VwCompileShader(Main Shaders/GBuffer/gbuffer.vert Shaders/GBuffer/gbuffer.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_colored.frag Shaders/GBuffer/gbuffer_colored.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_textured.frag Shaders/GBuffer/gbuffer_textured.spv)
VwCompileShader(Main Shaders/GBuffer/zpass.vert Shaders/GBuffer/zpass.spv)
VwCompileShader(Main Shaders/quad.vert Shaders/quad.spv)
VwCompileShader(Main Shaders/post-process/sky.frag Shaders/post-process/sky.spv)

# Ray tracing shaders
VwCompileShader(Main Shaders/RayTracing/raygen.rgen Shaders/RayTracing/raygen.rgen.spv --target-env vulkan1.2)
VwCompileShader(Main Shaders/RayTracing/miss.rmiss Shaders/RayTracing/miss.rmiss.spv --target-env vulkan1.2)
VwCompileShader(Main Shaders/RayTracing/hit.rchit Shaders/RayTracing/hit.rchit.spv --target-env vulkan1.2)

VwCompileShader(Main ../../VulkanWrapper/Shaders/fullscreen.vert Shaders/fullscreen.spv)
VwCompileShader(Main Shaders/post-process/sun_light.frag Shaders/post-process/sun_light.spv --target-env vulkan1.2)
VwCompileShader(Main Shaders/post-process/ambient_occlusion.frag Shaders/post-process/ambient_occlusion.spv --target-env vulkan1.2)
