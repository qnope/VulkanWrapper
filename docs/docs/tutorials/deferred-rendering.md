---
sidebar_position: 2
---

# Deferred Rendering

Implement a complete deferred rendering pipeline with VulkanWrapper.

## Overview

Deferred rendering separates geometry and lighting into distinct passes:

1. **G-Buffer Pass**: Render geometry to multiple render targets
2. **Lighting Pass**: Calculate lighting using G-Buffer data
3. **Post-Processing**: Apply effects and tone mapping

## G-Buffer Layout

Our G-Buffer contains:

| Buffer | Format | Content |
|--------|--------|---------|
| Position | R16G16B16A16Sfloat | World-space position |
| Normal | R16G16B16A16Sfloat | World-space normal |
| Albedo | R8G8B8A8Srgb | Base color |
| Depth | D32Sfloat | Depth buffer |

## Step 1: G-Buffer Pass

### Define Slots

```cpp
enum class GBufferSlot { Position, Normal, Albedo, Depth };

class GBufferPass : public Subpass<GBufferSlot> {
public:
    GBufferPass(std::shared_ptr<const Device> device,
                std::shared_ptr<const Allocator> allocator);

    struct Output {
        ImageView& position;
        ImageView& normal;
        ImageView& albedo;
        ImageView& depth;
    };

    Output render(CommandBuffer& cmd, const Scene& scene,
                  uint32_t width, uint32_t height, uint32_t frame);
};
```

### Implementation

```cpp
GBufferPass::Output GBufferPass::render(
    CommandBuffer& cmd, const Scene& scene,
    uint32_t width, uint32_t height, uint32_t frame)
{
    // Get or create G-Buffer images
    auto& position = get_or_create_image(
        GBufferSlot::Position, Width{width}, Height{height},
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled,
        frame);

    auto& normal = get_or_create_image(
        GBufferSlot::Normal, Width{width}, Height{height},
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled,
        frame);

    auto& albedo = get_or_create_image(
        GBufferSlot::Albedo, Width{width}, Height{height},
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled,
        frame);

    auto& depth = get_or_create_image(
        GBufferSlot::Depth, Width{width}, Height{height},
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment |
        vk::ImageUsageFlagBits::eSampled,
        frame);

    // Set up rendering
    std::array<vk::RenderingAttachmentInfo, 3> colorAttachments{{
        {position.handle(), vk::ImageLayout::eColorAttachmentOptimal,
         {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore},
        {normal.handle(), vk::ImageLayout::eColorAttachmentOptimal,
         {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore},
        {albedo.handle(), vk::ImageLayout::eColorAttachmentOptimal,
         {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore}
    }};

    vk::RenderingAttachmentInfo depthAttachment{
        depth.handle(), vk::ImageLayout::eDepthStencilAttachmentOptimal,
        {}, {}, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
        {.depthStencil = {1.0f, 0}}
    };

    vk::RenderingInfo renderInfo{
        .renderArea = {{0, 0}, {width, height}},
        .layerCount = 1,
        .colorAttachmentCount = 3,
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = &depthAttachment
    };

    cmd.handle().beginRendering(renderInfo);

    // Bind pipeline and render scene
    cmd.handle().bindPipeline(vk::PipelineBindPoint::eGraphics,
                              m_pipeline->handle());

    for (const auto& instance : scene.instances()) {
        // Set transform
        PushConstants pc{.model = instance.transform, .viewProj = m_viewProj};
        cmd.handle().pushConstants(m_layout.handle(),
                                   vk::ShaderStageFlagBits::eVertex,
                                   0, sizeof(pc), &pc);

        // Draw mesh
        cmd.handle().bindVertexBuffers(
            0, {instance.mesh->vertex_buffer().handle()}, {0});
        cmd.handle().bindIndexBuffer(
            instance.mesh->index_buffer().handle(), 0,
            vk::IndexType::eUint32);
        cmd.handle().drawIndexed(instance.mesh->index_count(), 1, 0, 0, 0);
    }

    cmd.handle().endRendering();

    return {position, normal, albedo, depth};
}
```

## Step 2: Lighting Pass

```cpp
class LightingPass : public ScreenSpacePass<LightingSlot> {
public:
    ImageView& render(CommandBuffer& cmd,
                      const GBufferPass::Output& gbuffer,
                      const std::vector<Light>& lights,
                      uint32_t width, uint32_t height, uint32_t frame);
};
```

### Lighting Shader

```glsl
#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gAlbedo;

layout(push_constant) uniform Light {
    vec3 position;
    vec3 color;
    float intensity;
} light;

void main() {
    vec3 fragPos = texture(gPosition, texCoord).xyz;
    vec3 normal = normalize(texture(gNormal, texCoord).xyz);
    vec3 albedo = texture(gAlbedo, texCoord).rgb;

    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (distance * distance);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color * light.intensity * attenuation;

    outColor = vec4(albedo * diffuse, 1.0);
}
```

## Step 3: Complete Pipeline

```cpp
class DeferredRenderer {
public:
    void render(CommandBuffer& cmd, const Scene& scene,
                uint32_t width, uint32_t height, uint32_t frame) {
        // 1. G-Buffer pass
        auto gbuffer = m_gbufferPass.render(cmd, scene, width, height, frame);

        // Transition G-Buffer for reading
        m_tracker.track(/* G-Buffer write states */);
        m_tracker.request(/* G-Buffer read states */);
        m_tracker.flush(cmd);

        // 2. Sky background
        auto& sky = m_skyPass.render(cmd, width, height, frame,
                                     m_skyParams, m_camera.view, m_camera.proj);

        // 3. Lighting
        auto& lighting = m_lightingPass.render(cmd, gbuffer, m_lights,
                                               width, height, frame);

        // 4. Tone mapping
        auto& final = m_toneMappingPass.render(cmd, width, height, frame,
                                               lighting,
                                               ToneMapOperator::ACES);

        // 5. Copy to swapchain
        // ...
    }

private:
    GBufferPass m_gbufferPass;
    SkyPass m_skyPass;
    LightingPass m_lightingPass;
    ToneMappingPass m_toneMappingPass;
    ResourceTracker m_tracker;
};
```

## Step 4: Adding Ray-Traced Shadows

```cpp
// In lighting pass, use ray-traced shadows
auto& sunLight = m_sunLightPass.render(
    cmd, width, height, frame,
    gbuffer.position, gbuffer.normal,
    m_tlas,  // Ray tracing acceleration structure
    m_skyParams
);
```

## Best Practices

1. **Use appropriate formats** for G-Buffer data
2. **Minimize G-Buffer bandwidth** with packed formats
3. **Consider depth prepass** for complex scenes
4. **Use light volumes** for many lights
5. **Apply MSAA** at G-Buffer level if needed
