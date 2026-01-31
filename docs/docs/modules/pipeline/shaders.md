---
sidebar_position: 3
---

# Shaders

VulkanWrapper provides shader compilation and module management.

## Overview

```cpp
#include <VulkanWrapper/Shader/ShaderCompiler.h>
#include <VulkanWrapper/Pipeline/ShaderModule.h>

// Compile from file
auto shader = ShaderCompiler::compile(
    device,
    "shaders/mesh.vert",
    vk::ShaderStageFlagBits::eVertex
);
```

## ShaderCompiler

### Compile from File

```cpp
auto shader = ShaderCompiler::compile(
    device,
    "path/to/shader.vert",
    vk::ShaderStageFlagBits::eVertex
);
```

### Compile from Source

```cpp
std::string source = R"(
    #version 450
    layout(location = 0) out vec4 outColor;
    void main() {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

auto shader = ShaderCompiler::compile(
    device,
    source,
    "inline_shader",  // Name for error messages
    vk::ShaderStageFlagBits::eFragment
);
```

### With Include Paths

```cpp
auto shader = ShaderCompiler::compile(
    device,
    "shaders/main.vert",
    vk::ShaderStageFlagBits::eVertex,
    {"shaders/include", "common/glsl"}
);
```

## Shader Stages

| Stage | Flag | Extension |
|-------|------|-----------|
| Vertex | `eVertex` | `.vert` |
| Fragment | `eFragment` | `.frag` |
| Compute | `eCompute` | `.comp` |
| Geometry | `eGeometry` | `.geom` |
| Tessellation Control | `eTessellationControl` | `.tesc` |
| Tessellation Eval | `eTessellationEvaluation` | `.tese` |
| Ray Gen | `eRaygenKHR` | `.rgen` |
| Ray Miss | `eMissKHR` | `.rmiss` |
| Ray Closest Hit | `eClosestHitKHR` | `.rchit` |
| Ray Any Hit | `eAnyHitKHR` | `.rahit` |
| Ray Intersection | `eIntersectionKHR` | `.rint` |

## ShaderModule

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::ShaderModule` | Get raw handle |
| `stage()` | `vk::ShaderStageFlagBits` | Get shader stage |

### Usage with Pipeline

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
    .build();
```

## GLSL Conventions

### Vertex Shader

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
} pc;

void main() {
    gl_Position = pc.viewProj * pc.model * vec4(inPosition, 1.0);
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;
}
```

### Fragment Shader

```glsl
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    outColor = texColor;
}
```

## Include System

VulkanWrapper shaders use includes for shared code:

### Include Paths

```
VulkanWrapper/Shaders/include/
├── atmosphere_params.glsl
├── atmosphere_scattering.glsl
├── sky_radiance.glsl
└── random.glsl
```

### Using Includes

```glsl
#version 450
#extension GL_GOOGLE_include_directive : require

#include "atmosphere_params.glsl"
#include "sky_radiance.glsl"

void main() {
    vec3 radiance = compute_sky_radiance(params, direction);
}
```

## Fullscreen Shader

Common fullscreen triangle vertex shader:

```glsl
// VulkanWrapper/Shaders/fullscreen.vert
#version 450

layout(location = 0) out vec2 texCoord;

void main() {
    texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
}
```

Usage:
```cpp
// Draw fullscreen without vertex buffer
cmd.draw(3, 1, 0, 0);
```

## Error Handling

Compilation errors throw `ShaderCompilationException`:

```cpp
try {
    auto shader = ShaderCompiler::compile(device, "bad.vert",
                                          vk::ShaderStageFlagBits::eVertex);
} catch (const ShaderCompilationException& e) {
    std::cerr << "Shader error: " << e.what() << '\n';
    // Includes line numbers and error details
}
```

## Best Practices

1. **Use includes** for shared code
2. **Match locations** between shader stages
3. **Match descriptor bindings** with layout
4. **Use push constants** for frequently changing data
5. **Cache compiled shaders** for faster startup
6. **Enable validation** to catch binding mismatches
