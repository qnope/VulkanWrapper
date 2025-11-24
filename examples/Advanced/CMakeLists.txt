
add_executable(Main main.cpp
    RenderPassInformation.h
    ZPass.h
    ColorPass.h
)

target_link_libraries(Main PRIVATE App)
target_precompile_headers(Main REUSE_FROM VulkanWrapperCoreLibrary)

VwCompileShader(Main Shaders/GBuffer/gbuffer.vert Shaders/GBuffer/gbuffer.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_colored.frag Shaders/GBuffer/gbuffer_colored.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_textured.frag Shaders/GBuffer/gbuffer_textured.spv)
VwCompileShader(Main Shaders/GBuffer/zpass.vert Shaders/GBuffer/zpass.spv)
VwCompileShader(Main Shaders/quad.vert Shaders/quad.spv)
VwCompileShader(Main Shaders/post-process/tonemap.frag Shaders/post-process/tonemap.spv)
VwCompileShader(Main Shaders/post-process/sky.frag Shaders/post-process/sky.spv)

# Ray tracing shaders
VwCompileShader(Main Shaders/RayTracing/raygen.rgen Shaders/RayTracing/raygen.rgen.spv --target-env vulkan1.2)
VwCompileShader(Main Shaders/RayTracing/miss.rmiss Shaders/RayTracing/miss.rmiss.spv --target-env vulkan1.2)
VwCompileShader(Main Shaders/RayTracing/hit.rchit Shaders/RayTracing/hit.rchit.spv --target-env vulkan1.2)
