---
sidebar_position: 3
---

# Ray-Traced Shadows

Add hardware ray-traced shadows to your renderer.

## Overview

Ray-traced shadows provide:
- Accurate hard shadows
- No shadow map artifacts
- Correct self-shadowing
- Arbitrary light shapes (with soft shadows)

## Prerequisites

- GPU with ray tracing support
- Built BLAS/TLAS for scene geometry
- Basic understanding of ray tracing concepts

## Step 1: Enable Ray Tracing

```cpp
// Request ray tracing extensions
auto device = instance->findGpu()
    .requireGraphicsQueue()
    .requireRayTracingExtensions()
    .find()
    .build();
```

## Step 2: Build Acceleration Structures

```cpp
#include <VulkanWrapper/RayTracing/RayTracedScene.h>

RayTracedScene rtScene(device, allocator);

// Add meshes
for (const auto& instance : scene.instances()) {
    rtScene.add_instance(instance.mesh, instance.transform);
}

// Build acceleration structures
rtScene.build(cmd);
```

## Step 3: Create Shadow Ray Pipeline

### Ray Generation Shader

For screen-space shadow rays:

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1) uniform sampler2D gPosition;
layout(set = 0, binding = 2, r8) uniform image2D shadowMask;

layout(push_constant) uniform Params {
    vec3 lightDirection;
    float maxDistance;
} params;

layout(location = 0) rayPayloadEXT float shadow;

void main() {
    ivec2 pixelCoord = ivec2(gl_LaunchIDEXT.xy);
    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(gl_LaunchSizeEXT.xy);

    vec3 worldPos = texture(gPosition, uv).xyz;

    // Skip background pixels
    if (worldPos == vec3(0)) {
        imageStore(shadowMask, pixelCoord, vec4(1.0));
        return;
    }

    // Trace shadow ray toward light
    shadow = 1.0;  // Assume lit

    traceRayEXT(
        tlas,
        gl_RayFlagsTerminateOnFirstHitEXT |
        gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF,
        0, 0, 0,  // SBT indices
        worldPos + params.lightDirection * 0.001,  // Origin (offset to avoid self-intersection)
        0.0,
        params.lightDirection,
        params.maxDistance,
        0  // Payload location
    );

    imageStore(shadowMask, pixelCoord, vec4(shadow));
}
```

### Miss Shader

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT float shadow;

void main() {
    // Ray missed - point is lit
    shadow = 1.0;
}
```

### Any-Hit Shader (Optional, for transparency)

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT float shadow;

void main() {
    // Hit geometry - point is in shadow
    shadow = 0.0;
    terminateRayEXT;
}
```

## Step 4: Create Pipeline

```cpp
auto shadowPipeline = RayTracingPipelineBuilder()
    .setDevice(device)
    .setLayout(pipelineLayout)
    .addRaygenShader(shadowRaygenShader)
    .addMissShader(shadowMissShader)
    .addHitGroup(nullptr)  // No closest hit needed
    .build();

auto sbt = shadowPipeline->create_sbt(allocator);
```

## Step 5: Shadow Pass Implementation

```cpp
class ShadowPass : public ScreenSpacePass<ShadowSlot> {
public:
    ImageView& render(CommandBuffer& cmd,
                      const ImageView& positions,
                      const TopLevelAccelerationStructure& tlas,
                      const glm::vec3& lightDirection,
                      uint32_t width, uint32_t height,
                      uint32_t frame) {
        // Get shadow mask image
        auto& shadowMask = get_or_create_image(
            ShadowSlot::Mask, Width{width}, Height{height},
            vk::Format::eR8Unorm,
            vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eSampled,
            frame);

        // Update descriptors
        updateDescriptors(tlas, positions, shadowMask);

        // Bind pipeline
        cmd.handle().bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                                  m_pipeline->handle());
        cmd.handle().bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            m_layout.handle(), 0, m_descriptorSet, {});

        // Push constants
        ShadowParams params{
            .lightDirection = glm::normalize(lightDirection),
            .maxDistance = 1000.0f
        };
        cmd.handle().pushConstants(m_layout.handle(),
                                   vk::ShaderStageFlagBits::eRaygenKHR,
                                   0, sizeof(params), &params);

        // Trace rays
        cmd.handle().traceRaysKHR(
            m_sbt.raygen_region(),
            m_sbt.miss_region(),
            m_sbt.hit_region(),
            m_sbt.callable_region(),
            width, height, 1);

        return shadowMask;
    }
};
```

## Step 6: Apply Shadows in Lighting

```glsl
// In lighting shader
layout(set = 1, binding = 0) uniform sampler2D shadowMask;

void main() {
    float shadow = texture(shadowMask, texCoord).r;

    // Apply shadow to lighting
    vec3 lighting = diffuse * shadow + ambient;
    outColor = vec4(albedo * lighting, 1.0);
}
```

## Soft Shadows

For soft shadows, trace multiple rays:

```glsl
// In raygen shader
float shadow = 0.0;
const int numSamples = 16;

for (int i = 0; i < numSamples; i++) {
    // Jitter ray direction for soft shadows
    vec3 jitteredDir = jitterDirection(params.lightDirection, i, numSamples);

    float sampleShadow = 1.0;
    traceRayEXT(tlas, flags, 0xFF, 0, 0, 0,
                origin, 0.0, jitteredDir, maxDist, 0);

    shadow += sampleShadow;
}

shadow /= float(numSamples);
```

## Best Practices

1. **Use ray flags** to skip closest hit shaders for shadow rays
2. **Offset ray origin** to prevent self-intersection
3. **Consider temporal accumulation** for soft shadows
4. **Use any-hit** for transparent shadow casters
5. **Limit shadow distance** for performance
