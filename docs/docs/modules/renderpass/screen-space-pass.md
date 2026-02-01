---
sidebar_position: 2
---

# Screen-Space Pass

The `ScreenSpacePass` class provides utilities for full-screen post-processing effects.

## Overview

```cpp
#include <VulkanWrapper/RenderPass/ScreenSpacePass.h>

class BlurPass : public ScreenSpacePass<BlurSlot> {
public:
    BlurPass(std::shared_ptr<const Device> device);

    ImageView& render(CommandBuffer& cmd,
                      const ImageView& input,
                      uint32_t width, uint32_t height);
};
```

## ScreenSpacePass Template

Extends `Subpass` with screen-space utilities:

```cpp
template <typename SlotEnum>
class ScreenSpacePass : public Subpass<SlotEnum> {
protected:
    // Create default sampler (linear, clamp to edge)
    static std::shared_ptr<Sampler> create_default_sampler(
        std::shared_ptr<const Device> device);

    // Render fullscreen quad
    void render_fullscreen(
        CommandBuffer& cmd,
        const Pipeline& pipeline,
        const ImageView& output,
        vk::ImageLayout outputLayout,
        const DescriptorSet& descriptorSet,
        std::span<const std::byte> pushConstants = {}
    );
};
```

## Helper Functions

### create_default_sampler

Creates a sampler optimized for screen-space effects:

```cpp
auto sampler = create_default_sampler(device);
// - Linear filtering
// - Clamp to edge addressing
// - No mipmapping
```

### render_fullscreen

Handles the common fullscreen rendering pattern:

```cpp
void render_fullscreen(
    CommandBuffer& cmd,
    const Pipeline& pipeline,
    const ImageView& output,
    vk::ImageLayout outputLayout,
    const DescriptorSet& descriptorSet,
    std::span<const std::byte> pushConstants = {}
);
```

This function:
1. Begins rendering with the output attachment
2. Sets viewport and scissor
3. Binds pipeline and descriptor set
4. Pushes constants (if provided)
5. Draws fullscreen triangle (3 vertices)
6. Ends rendering

## create_screen_space_pipeline

Factory function for screen-space pipelines:

```cpp
auto pipeline = create_screen_space_pipeline(
    device,
    layout,
    fragmentShader,
    vk::Format::eR16G16B16A16Sfloat,  // Output format
    std::nullopt                       // No blending
);
```

With blending:

```cpp
auto pipeline = create_screen_space_pipeline(
    device,
    layout,
    fragmentShader,
    vk::Format::eR16G16B16A16Sfloat,
    ColorBlendConfig::additive()
);
```

## Fullscreen Vertex Shader

VulkanWrapper provides a standard fullscreen vertex shader:

```glsl
// VulkanWrapper/Shaders/fullscreen.vert
#version 450

layout(location = 0) out vec2 texCoord;

void main() {
    texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
}
```

This generates a fullscreen triangle without vertex buffers.

## Implementation Example

```cpp
class BloomPass : public ScreenSpacePass<BloomSlot> {
public:
    BloomPass(std::shared_ptr<const Device> device,
              std::shared_ptr<const Allocator> allocator)
        : m_device(device)
        , m_allocator(allocator)
    {
        // Create sampler
        m_sampler = create_default_sampler(device);

        // Create descriptor layout
        m_layout = DescriptorSetLayoutBuilder()
            .setDevice(device)
            .addBinding(0, vk::DescriptorType::eCombinedImageSampler,
                        vk::ShaderStageFlagBits::eFragment)
            .build();

        // Compile shaders
        auto vertShader = ShaderCompiler::compile(
            device, "fullscreen.vert",
            vk::ShaderStageFlagBits::eVertex);
        auto fragShader = ShaderCompiler::compile(
            device, "bloom.frag",
            vk::ShaderStageFlagBits::eFragment);

        // Create pipeline
        auto pipelineLayout = PipelineLayoutBuilder()
            .setDevice(device)
            .addDescriptorSetLayout(m_layout)
            .build();

        m_pipeline = create_screen_space_pipeline(
            device,
            pipelineLayout,
            fragShader,
            vk::Format::eR16G16B16A16Sfloat,
            ColorBlendConfig::additive()
        );

        // Create descriptor allocator
        m_descriptorAllocator = std::make_unique<DescriptorAllocator>(device);
    }

    ImageView& render(CommandBuffer& cmd,
                      const ImageView& input,
                      uint32_t width, uint32_t height,
                      uint32_t frameIndex) {
        // Get output image
        auto& output = get_or_create_image(
            BloomSlot::Output,
            Width{width}, Height{height},
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled,
            frameIndex
        );

        // Update descriptor
        vk::DescriptorImageInfo imageInfo{
            .sampler = m_sampler->handle(),
            .imageView = input.handle(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        auto descriptorSet = m_descriptorAllocator->allocate(
            m_layout, {{0, imageInfo}});

        // Render fullscreen
        render_fullscreen(cmd, *m_pipeline, output,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          *descriptorSet);

        return output;
    }

private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    std::shared_ptr<Sampler> m_sampler;
    std::shared_ptr<DescriptorSetLayout> m_layout;
    std::shared_ptr<const Pipeline> m_pipeline;
    std::unique_ptr<DescriptorAllocator> m_descriptorAllocator;
};
```

## Best Practices

1. **Use create_default_sampler** for consistent sampling
2. **Use fullscreen.vert** to avoid vertex buffer overhead
3. **Chain passes** by passing ImageView references
4. **Use HDR formats** (R16G16B16A16Sfloat) for lighting
5. **Consider blending** for additive effects
