
add_executable(Main main.cpp ZPass.h ColorPass.h)
target_link_libraries(Main PRIVATE App)

VwCompileShader(Main Shaders/GBuffer/gbuffer.vert Shaders/gbuffer.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_colored.frag Shaders/gbuffer_colored.spv)
VwCompileShader(Main Shaders/GBuffer/gbuffer_textured.frag Shaders/gbuffer_textured.spv)
VwCompileShader(Main Shaders/GBuffer/zpass.vert Shaders/zpass.spv)
