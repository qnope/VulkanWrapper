
add_executable(Main main.cpp
    ZPass.h
    ColorPass.h
    TonemapPass.h
    SkyPass.h)
target_link_libraries(Main PRIVATE App)

VwCompileShader(Main Shaders/GBuffer/gbuffer.vert Shaders/GBuffer/gbuffer.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_colored.frag Shaders/GBuffer/gbuffer_colored.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_textured.frag Shaders/GBuffer/gbuffer_textured.spv)
VwCompileShader(Main Shaders/GBuffer/zpass.vert Shaders/GBuffer/zpass.spv)
VwCompileShader(Main Shaders/quad.vert Shaders/quad.spv)
VwCompileShader(Main Shaders/post-process/tonemap.frag Shaders/post-process/tonemap.spv)
VwCompileShader(Main Shaders/post-process/sky.frag Shaders/post-process/sky.spv)
