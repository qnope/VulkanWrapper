---
sidebar_position: 2
---

# Hello Texture

Learn to load and display textures using VulkanWrapper.

## Overview

This tutorial teaches you to:
- Create and upload texture data to the GPU
- Set up samplers for texture filtering
- Use descriptor sets to bind textures to shaders
- Sample textures in fragment shaders

## Architecture

```mermaid
flowchart TB
    subgraph "CPU Side"
        Pixels[Pixel Data]
        Vertices[Vertex Data]
    end

    subgraph "Upload Phase"
        Staging[Staging Buffer]
        Transfer[Transfer Command]
    end

    subgraph "GPU Resources"
        Image[GPU Image]
        View[Image View]
        Sampler[Sampler]
        Combined[Combined Image Sampler]
    end

    subgraph "Binding"
        Layout[Descriptor Set Layout]
        Pool[Descriptor Pool]
        Set[Descriptor Set]
    end

    subgraph "Render"
        Pipeline[Graphics Pipeline]
        Fragment[Fragment Shader]
    end

    Pixels --> Staging
    Staging --> Transfer
    Transfer --> Image

    Image --> View
    View --> Combined
    Sampler --> Combined
    Combined --> Set

    Layout --> Set
    Pool --> Set
    Set --> Fragment
    Pipeline --> Fragment
```

## Concepts

### Texture Pipeline

Textures in Vulkan require several components working together:

```mermaid
classDiagram
    class Image {
        +VkImage handle
        +VkFormat format
        +Extent dimensions
    }

    class ImageView {
        +VkImageView handle
        +ViewType type
        +SubresourceRange range
    }

    class Sampler {
        +VkSampler handle
        +Filter magFilter
        +Filter minFilter
        +AddressMode addressMode
    }

    class CombinedImage {
        +Image image
        +ImageView view
        +Sampler sampler
    }

    CombinedImage --> Image
    CombinedImage --> ImageView
    CombinedImage --> Sampler
```

### Descriptor Sets

Descriptor sets bind resources (textures, buffers) to shader bindings:

```mermaid
flowchart LR
    subgraph "CPU"
        App[Application]
    end

    subgraph "Descriptor System"
        Layout[Layout: binding 0 = sampler2D]
        Pool[Pool: 1 combined image sampler]
        Set[Descriptor Set]
    end

    subgraph "Shader"
        Binding["layout(binding = 0) uniform sampler2D"]
    end

    App --> Layout
    Layout --> Pool
    Pool --> Set
    Set --> Binding
```

## Implementation

### Step 1: Create Texture Data

Generate a checkerboard pattern in CPU memory:

```cpp
constexpr uint32_t texWidth = 64;
constexpr uint32_t texHeight = 64;
constexpr uint32_t checkerSize = 8;

std::vector<uint8_t> pixels(texWidth * texHeight * 4);
for (uint32_t y = 0; y < texHeight; ++y) {
    for (uint32_t x = 0; x < texWidth; ++x) {
        uint32_t index = (y * texWidth + x) * 4;
        bool isWhite = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;

        pixels[index + 0] = isWhite ? 255 : 50;  // R
        pixels[index + 1] = isWhite ? 255 : 50;  // G
        pixels[index + 2] = isWhite ? 255 : 50;  // B
        pixels[index + 3] = 255;                  // A
    }
}
```

### Step 2: Create GPU Image

Create an image that can be sampled and receive transfer data:

```cpp
auto image = allocator()->create_image(
    vk::Format::eR8G8B8A8Unorm,
    Width{texWidth},
    Height{texHeight},
    vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferDst);
```

### Step 3: Upload Texture Data

Use the staging buffer system to upload pixel data:

```mermaid
sequenceDiagram
    participant App
    participant Staging as StagingBufferManager
    participant GPU

    App->>Staging: stage_image(image, pixels)
    Note over Staging: Copies to staging buffer

    App->>GPU: Record transfer commands
    App->>Staging: flush(cmd)
    Note over GPU: Execute copy

    GPU-->>App: Texture ready
```

```cpp
StagingBufferManager staging(*allocator());
staging.stage_image(*image, std::span(pixels));

// Record and execute upload commands
{
    CommandBufferRecorder recorder(cmd);
    transfer().resourceTracker().flush(cmd);
    staging.flush(cmd);
}

// Submit and wait
queue().handle().submit2(submitInfo, fence.handle());
fence.wait();
```

### Step 4: Create Image View and Sampler

```cpp
// Image view - allows shader to access the image
auto imageView = ImageViewBuilder(device(), image)
    .as_2d()
    .build();

// Sampler - defines filtering and addressing
auto sampler = SamplerBuilder(device())
    .set_filter(vk::Filter::eNearest, vk::Filter::eNearest)
    .set_address_mode(vk::SamplerAddressMode::eRepeat)
    .build();
```

**Sampler Filters:**

| Filter | Effect |
|--------|--------|
| `eNearest` | Pixelated look, no interpolation |
| `eLinear` | Smooth interpolation between texels |

**Address Modes:**

| Mode | Effect |
|------|--------|
| `eRepeat` | Tile texture |
| `eClampToEdge` | Stretch edge pixels |
| `eMirroredRepeat` | Tile with mirroring |

### Step 5: Create Descriptor Set

Set up the binding between texture and shader:

```cpp
// Define what the shader expects
m_descriptorSetLayout = DescriptorSetLayoutBuilder(device())
    .add_binding(0, vk::DescriptorType::eCombinedImageSampler,
                 vk::ShaderStageFlagBits::eFragment)
    .build();

// Create pool with enough descriptors
m_descriptorPool = std::make_unique<DescriptorPool>(
    device(), 1,
    std::vector{vk::DescriptorPoolSize{
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1}});

// Allocate and update descriptor set
m_descriptorSet = std::make_unique<DescriptorSet>(
    m_descriptorPool->allocate(m_descriptorSetLayout->handle()));

m_descriptorSet->update_combined_image_sampler(
    0,  // binding
    sampler.handle(),
    imageView.handle(),
    vk::ImageLayout::eShaderReadOnlyOptimal);
```

### Step 6: Sample in Shader

The fragment shader samples the texture using the bound sampler:

```glsl
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```

### Step 7: Bind and Draw

During rendering, bind the descriptor set and draw:

```cpp
void render(vk::CommandBuffer cmd, uint32_t frameIndex) {
    // Ensure texture is in correct layout
    transfer().resourceTracker().request(Barrier::ImageState{
        .image = m_combinedImage->image().handle(),
        .subresourceRange = m_combinedImage->image().full_range(),
        .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .access = vk::AccessFlagBits2::eShaderSampledRead});
    transfer().resourceTracker().flush(cmd);

    beginRendering(cmd);

    // Bind pipeline and descriptors
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline->handle());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           m_pipelineLayout->handle(), 0,
                           m_descriptorSet->handle(), {});

    cmd.bindVertexBuffers(0, m_vertexBuffer->handle(), vk::DeviceSize{0});
    cmd.draw(6, 1, 0, 0);

    endRendering(cmd);
}
```

## Result

![Hello Texture Screenshot](/img/tutorials/hello-texture.png)

The output shows a quad with a black and white checkerboard texture.

## Resource Flow Summary

```mermaid
flowchart TD
    subgraph "Creation"
        A1[Generate pixel data] --> A2[Create GPU Image]
        A2 --> A3[Upload via Staging]
        A3 --> A4[Create View + Sampler]
    end

    subgraph "Binding"
        B1[Create Descriptor Layout] --> B2[Create Descriptor Pool]
        B2 --> B3[Allocate Descriptor Set]
        B3 --> B4[Update with Texture]
    end

    subgraph "Rendering"
        C1[Bind Descriptor Set] --> C2[Draw]
        C2 --> C3[Shader Samples Texture]
    end

    A4 --> B4
    B4 --> C1
```

## Full Source Code

The complete source code is available at:
`docs/tutorials/02-texture/main.cpp`

## Key Takeaways

1. **CombinedImage**: Groups Image, ImageView, and Sampler together
2. **StagingBufferManager**: Handles CPU-to-GPU data transfer
3. **Descriptor Sets**: Bind resources to shader bindings
4. **Resource Tracking**: Ensures correct image layouts for each operation

## Next Steps

- [Hello 3D](./hello-3d) - Add 3D transformations
- [Images](./images) - Deep dive into image handling
- [Descriptors](./descriptors) - Advanced descriptor management
