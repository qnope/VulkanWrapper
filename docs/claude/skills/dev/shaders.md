# Shaders

Shader compilation, module creation, and GLSL include system.

## ShaderModule

Load pre-compiled SPIR-V shaders:

```cpp
#include "VulkanWrapper/Pipeline/ShaderModule.h"

// From SPIR-V file
auto module = ShaderModule::create_from_spirv_file(device, "shaders/main.vert.spv");

// From in-memory SPIR-V
std::vector<uint32_t> spirv = {...};
auto module = ShaderModule::create_from_spirv(device, spirv);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Pipeline/ShaderModule.h`

---

## ShaderCompiler

Runtime GLSL-to-SPIR-V compilation:

```cpp
#include "VulkanWrapper/Shader/ShaderCompiler.h"

// Basic compilation
auto compiler = ShaderCompiler();
auto result = compiler.compile_from_file("shaders/main.frag");
std::vector<uint32_t> spirv = result.spirv;

// Full configuration
auto compiler = ShaderCompiler()
    .add_include_path("shaders/include")
    .add_include_path("VulkanWrapper/Shaders/include")
    .set_target_vulkan_version(vk::ApiVersion13)
    .add_macro("DEBUG", "1")
    .set_optimize(true);

// Compile to module directly
auto module = compiler.compile_file_to_module(device, "shaders/main.vert");
```

### Stage Detection

Shader stage is detected from file extension:

| Extension | Stage |
|-----------|-------|
| `.vert` | Vertex |
| `.frag` | Fragment |
| `.comp` | Compute |
| `.rgen` | Ray Generation |
| `.rmiss` | Ray Miss |
| `.rchit` | Ray Closest Hit |
| `.rahit` | Ray Any Hit |
| `.rint` | Ray Intersection |
| `.rcall` | Ray Callable |
| `.geom` | Geometry |
| `.tesc` | Tessellation Control |
| `.tese` | Tessellation Evaluation |

Or specify explicitly:

```cpp
auto result = compiler.compile(source, vk::ShaderStageFlagBits::eFragment);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Shader/ShaderCompiler.h`

---

## GLSL Include System

### Project Include Paths

The project provides shared GLSL includes:

```
VulkanWrapper/Shaders/include/
├── atmosphere_params.glsl    # SkyParameters struct
├── atmosphere_scattering.glsl # Rayleigh/Mie functions
├── sky_radiance.glsl         # Sky color computation
└── random.glsl               # Random sampling utilities
```

### Using Includes

```glsl
// In your shader
#include "atmosphere_params.glsl"
#include "random.glsl"

// Must set up compiler with include paths
auto compiler = ShaderCompiler()
    .add_include_path("VulkanWrapper/Shaders/include")
    .add_include_path("your/project/shaders/include");
```

### Virtual Includes

Add includes from memory without files:

```cpp
auto compiler = ShaderCompiler()
    .add_include("common/constants.glsl", R"(
        #define PI 3.14159265359
        #define EPSILON 0.0001
    )");
```

---

## Random Sampling Include

The `random.glsl` include provides hemisphere sampling utilities:

```glsl
// Define bindings BEFORE including
#define RANDOM_XI_BUFFER_BINDING 8
#define RANDOM_NOISE_TEXTURE_BINDING 9
#include "random.glsl"

void main() {
    // Get decorrelated random sample for this pixel
    vec2 xi = get_sample(sampleIndex, ivec2(gl_FragCoord.xy));

    // Cosine-weighted hemisphere sample
    vec3 direction = sample_hemisphere_cosine(normal, tangent, bitangent, xi);

    // PDF for importance sampling
    float pdf = pdf_hemisphere_cosine(dot(direction, normal));
}
```

**See:** [memory-allocation.md](memory-allocation.md#random-sampling) for buffer setup.

---

## Pipeline Integration

### Graphics Pipeline

```cpp
auto vertShader = compiler.compile_file_to_module(device, "shaders/main.vert");
auto fragShader = compiler.compile_file_to_module(device, "shaders/main.frag");

auto pipeline = GraphicsPipelineBuilder(device, pipelineLayout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
    .set_vertex_input<Vertex3D>()
    // ... other configuration
    .build();
```

### Compute Pipeline

```cpp
auto compShader = compiler.compile_file_to_module(device, "shaders/process.comp");

vk::ComputePipelineCreateInfo createInfo{
    .stage = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = compShader->handle(),
        .pName = "main"
    },
    .layout = pipelineLayout.handle()
};

auto pipeline = device->handle().createComputePipelineUnique(nullptr, createInfo);
```

### Ray Tracing Pipeline

See [ray-tracing.md](ray-tracing.md) for RT shader usage.

---

## Push Constants

Push constants are the preferred way to pass small, frequently-changing data:

### Layout Definition

```cpp
struct PushConstants {
    glm::mat4 viewProj;
    glm::vec3 cameraPos;
    float time;
};

auto layout = PipelineLayoutBuilder(device)
    .add_descriptor_set_layout(descriptorLayout)
    .add_push_constant_range<PushConstants>(
        vk::ShaderStageFlagBits::eVertex |
        vk::ShaderStageFlagBits::eFragment)
    .build();
```

### GLSL Declaration

```glsl
layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec3 cameraPos;
    float time;
} pc;

void main() {
    vec4 worldPos = pc.viewProj * vec4(position, 1.0);
    float dist = length(pc.cameraPos - position);
}
```

### Usage in Rendering

```cpp
PushConstants pc{
    .viewProj = camera.viewProj(),
    .cameraPos = camera.position(),
    .time = currentTime
};

cmd.pushConstants(
    layout.handle(),
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
    0, sizeof(pc), &pc
);
```

### Push Constant Limits

- Maximum size: 128 bytes (guaranteed minimum)
- Query actual limit: `VkPhysicalDeviceProperties::limits::maxPushConstantsSize`

---

## Descriptor Binding Conventions

### Layout Organization

```glsl
// Set 0: Global/scene data (rarely changes)
layout(set = 0, binding = 0) uniform SceneUBO { ... };
layout(set = 0, binding = 1) uniform accelerationStructureEXT tlas;

// Set 1: Per-material data
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;

// Set 2: Per-draw data (dynamic)
layout(set = 2, binding = 0) uniform ModelUBO { mat4 model; };
```

### Storage Buffers

```glsl
// Read-only storage buffer
layout(set = 0, binding = 0, std430) readonly buffer Vertices {
    vec4 positions[];
};

// Read-write storage buffer
layout(set = 0, binding = 1, std430) buffer Output {
    vec4 results[];
};
```

### Combined Image Samplers

```glsl
layout(set = 0, binding = 0) uniform sampler2D colorTexture;

void main() {
    vec4 color = texture(colorTexture, uv);
}
```

---

## Fullscreen Shader Pattern

The project provides a shared fullscreen vertex shader:

```glsl
// VulkanWrapper/Shaders/fullscreen.vert
#version 460

layout(location = 0) out vec2 outUV;

void main() {
    // Generate fullscreen triangle from vertex ID
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);
}
```

Usage:

```cpp
// No vertex buffer needed!
cmd.draw(3, 1, 0, 0);  // Draw fullscreen triangle
```

---

## Compilation Error Handling

```cpp
try {
    auto module = compiler.compile_file_to_module(device, "shaders/broken.frag");
} catch (const vw::ShaderCompilationException& e) {
    std::cerr << "Shader compilation failed:\n"
              << e.what() << "\n"
              << "at " << e.location().file_name()
              << ":" << e.location().line() << "\n";
}
```

---

## Anti-Patterns

```cpp
// DON'T: Forget include paths
auto compiler = ShaderCompiler();
compiler.compile_from_file("shader_with_includes.frag");  // Fails!

// DO: Add include paths first
auto compiler = ShaderCompiler()
    .add_include_path("shaders/include")
    .add_include_path("VulkanWrapper/Shaders/include");

// DON'T: Put large data in push constants
struct BadPushConstants {
    glm::mat4 matrices[16];  // 1024 bytes - exceeds limit!
};

// DO: Use uniform buffers for large data
struct GoodPushConstants {
    glm::mat4 viewProj;  // 64 bytes
    glm::vec4 params;    // 16 bytes
};  // Total: 80 bytes, well under 128

// DON'T: Recompile shaders every frame
while (running) {
    auto shader = compiler.compile_file_to_module(device, "shader.frag");
    // Very expensive!
}

// DO: Compile once, reuse
auto shader = compiler.compile_file_to_module(device, "shader.frag");
while (running) {
    // Use cached shader
}

// DON'T: Hardcode binding numbers inconsistently
// C++: binding 0     GLSL: binding 1  ← Mismatch!

// DO: Use shared header or constants
// constants.h: constexpr int SCENE_UBO_BINDING = 0;
// shader.glsl: layout(binding = 0) uniform SceneUBO {...};
```

---

## Key Source Files

| Purpose | Path |
|---------|------|
| ShaderModule | `VulkanWrapper/include/VulkanWrapper/Pipeline/ShaderModule.h` |
| ShaderCompiler | `VulkanWrapper/include/VulkanWrapper/Shader/ShaderCompiler.h` |
| Fullscreen vertex | `VulkanWrapper/Shaders/fullscreen.vert` |
| Random include | `VulkanWrapper/Shaders/include/random.glsl` |
| Atmosphere includes | `VulkanWrapper/Shaders/include/atmosphere_*.glsl` |
