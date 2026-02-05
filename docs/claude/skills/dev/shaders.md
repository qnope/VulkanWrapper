# Shaders

## Compilation

```cpp
auto compiler = ShaderCompiler()
    .add_include_path("VulkanWrapper/Shaders/include")
    .set_target_vulkan_version(vk::ApiVersion13);

auto module = compiler.compile_file_to_module(device, "shader.frag");
```

Stage detected from extension: `.vert`, `.frag`, `.comp`, `.rgen`, `.rmiss`, `.rchit`

## GLSL Includes

Available in `VulkanWrapper/Shaders/include/`:
- `random.glsl` - Hemisphere sampling
- `atmosphere_params.glsl` - Sky parameters
- `atmosphere_scattering.glsl` - Rayleigh/Mie

```glsl
#define RANDOM_XI_BUFFER_BINDING 8
#define RANDOM_NOISE_TEXTURE_BINDING 9
#include "random.glsl"

vec2 xi = get_sample(sampleIndex, ivec2(gl_FragCoord.xy));
vec3 dir = sample_hemisphere_cosine(normal, tangent, bitangent, xi);
```

## Push Constants

```cpp
// Layout
auto layout = PipelineLayoutBuilder(device)
    .add_push_constant_range<PushConstants>(vk::ShaderStageFlagBits::eVertex | eFragment)
    .build();

// Usage
cmd.pushConstants(layout.handle(), stages, 0, sizeof(pc), &pc);
```

Max size: 128 bytes (guaranteed minimum).

## Fullscreen Shader

`VulkanWrapper/Shaders/fullscreen.vert` - generates fullscreen quad via triangle strip from vertex ID.

```cpp
cmd.draw(4, 1, 0, 0);  // No vertex buffer needed, 4 vertices triangle strip
```
