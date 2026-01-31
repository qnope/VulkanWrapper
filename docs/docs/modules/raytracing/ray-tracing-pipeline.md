---
sidebar_position: 2
---

# Ray Tracing Pipeline

The `RayTracingPipeline` class manages Vulkan ray tracing pipelines.

## Overview

```cpp
#include <VulkanWrapper/RayTracing/RayTracingPipeline.h>

auto pipeline = RayTracingPipelineBuilder()
    .setDevice(device)
    .setLayout(pipelineLayout)
    .addRaygenShader(raygenShader)
    .addMissShader(missShader)
    .addHitGroup(closestHitShader)
    .build();
```

## Shader Types

Ray tracing pipelines use several shader stages:

| Stage | Purpose |
|-------|---------|
| Ray Generation | Launch rays, main entry point |
| Miss | Called when ray hits nothing |
| Closest Hit | Called for closest intersection |
| Any Hit | Called for any intersection (optional) |
| Intersection | Custom intersection test (optional) |

## RayTracingPipelineBuilder

### Adding Shaders

```cpp
auto builder = RayTracingPipelineBuilder()
    .setDevice(device)
    .setLayout(layout);

// Ray generation (required)
builder.addRaygenShader(raygenShader);

// Miss shaders
builder.addMissShader(primaryMissShader);
builder.addMissShader(shadowMissShader);

// Hit groups
builder.addHitGroup(closestHitShader);  // Triangles
builder.addHitGroup(closestHit, anyHit);  // With any-hit
builder.addHitGroup(closestHit, anyHit, intersection);  // Procedural

auto pipeline = builder.build();
```

### Pipeline Layout

```cpp
auto layout = PipelineLayoutBuilder()
    .setDevice(device)
    .addDescriptorSetLayout(sceneLayout)   // TLAS, buffers
    .addDescriptorSetLayout(outputLayout)  // Output image
    .addPushConstantRange<RayTracingParams>(
        vk::ShaderStageFlagBits::eRaygenKHR |
        vk::ShaderStageFlagBits::eClosestHitKHR)
    .build();
```

## RayTracingPipeline Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::Pipeline` | Get raw handle |
| `layout()` | `const PipelineLayout&` | Get pipeline layout |
| `create_sbt()` | `ShaderBindingTable` | Create shader binding table |

## Shader Binding Table

The SBT defines which shaders are called:

```cpp
auto sbt = pipeline->create_sbt(allocator);
```

See [Shader Binding Table](./shader-binding-table) for details.

## Tracing Rays

```cpp
// Bind pipeline
cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                 pipeline->handle());

// Bind descriptors
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eRayTracingKHR,
    pipeline->layout().handle(),
    0, descriptorSets, {}
);

// Push constants
cmd.pushConstants(
    pipeline->layout().handle(),
    vk::ShaderStageFlagBits::eRaygenKHR,
    0, sizeof(params), &params
);

// Trace rays
cmd.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    sbt.callable_region(),
    width, height, 1
);
```

## Shader Examples

### Ray Generation Shader

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba16f) uniform image2D outputImage;

layout(push_constant) uniform Params {
    mat4 viewInverse;
    mat4 projInverse;
} params;

layout(location = 0) rayPayloadEXT vec3 payload;

void main() {
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 uv = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = uv * 2.0 - 1.0;

    vec4 origin = params.viewInverse * vec4(0, 0, 0, 1);
    vec4 target = params.projInverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = params.viewInverse * vec4(normalize(target.xyz), 0);

    payload = vec3(0);

    traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF,
                0, 0, 0,
                origin.xyz, 0.001,
                direction.xyz, 10000.0,
                0);

    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(payload, 1));
}
```

### Miss Shader

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main() {
    // Sky color when ray misses geometry
    vec3 direction = normalize(gl_WorldRayDirectionEXT);
    float t = 0.5 * (direction.y + 1.0);
    payload = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
}
```

### Closest Hit Shader

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

hitAttributeEXT vec2 barycentrics;

void main() {
    // Compute barycentric coordinates
    const vec3 bary = vec3(1.0 - barycentrics.x - barycentrics.y,
                           barycentrics.x, barycentrics.y);

    // Use hit information
    payload = bary;  // Example: visualize barycentrics
}
```

## Best Practices

1. **Minimize ray depth** for performance
2. **Use miss shaders** for sky/background
3. **Consider any-hit** for transparency
4. **Batch similar rays** when possible
5. **Use ray flags** to skip unnecessary tests
