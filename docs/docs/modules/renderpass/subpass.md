---
sidebar_position: 1
---

# Subpass

The `Subpass` base class provides lazy image allocation with automatic caching.

## Overview

```cpp
#include <VulkanWrapper/RenderPass/Subpass.h>

// Define slot enum for your pass
enum class GBufferSlot { Position, Normal, Albedo };

class GBufferPass : public Subpass<GBufferSlot> {
public:
    void render(CommandBuffer& cmd, uint32_t width, uint32_t height,
                uint32_t frameIndex);
};
```

## Subpass Template

```cpp
template <typename SlotEnum>
class Subpass {
protected:
    ImageView& get_or_create_image(
        SlotEnum slot,
        Width width, Height height,
        vk::Format format,
        vk::ImageUsageFlags usage,
        uint32_t frameIndex
    );

    void clear_cache();
};
```

### Template Parameter

The `SlotEnum` defines the output slots for this pass:

```cpp
enum class ToneMappingSlot { Output };
enum class GBufferSlot { Position, Normal, Albedo, Depth };
enum class ShadowSlot { ShadowMap };
```

## Lazy Image Allocation

Images are created on first access and cached:

```cpp
class MyPass : public Subpass<MySlot> {
public:
    ImageView& render(uint32_t width, uint32_t height, uint32_t frame) {
        // Created on first call, reused on subsequent calls
        auto& output = get_or_create_image(
            MySlot::Output,
            Width{width}, Height{height},
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled,
            frame
        );

        // Render to output...

        return output;
    }
};
```

### Cache Key

Images are cached by:
- Slot enum value
- Width and height
- Frame index

When dimensions change, old images are replaced:

```cpp
// First call creates image
auto& view1 = get_or_create_image(Slot::A, Width{1920}, Height{1080}, ...);

// Same dimensions - returns cached image
auto& view2 = get_or_create_image(Slot::A, Width{1920}, Height{1080}, ...);
assert(&view1 == &view2);

// Different dimensions - creates new image
auto& view3 = get_or_create_image(Slot::A, Width{1280}, Height{720}, ...);
```

## Frame Indexing

For multi-buffered rendering:

```cpp
class MyPass : public Subpass<MySlot> {
public:
    ImageView& render(uint32_t frameIndex) {
        // Each frame gets its own image
        return get_or_create_image(
            MySlot::Output,
            width, height,
            format, usage,
            frameIndex  // Different cache entry per frame
        );
    }
};
```

## Implementing a Subpass

### Basic Structure

```cpp
class ColorPass : public Subpass<ColorSlot> {
public:
    ColorPass(std::shared_ptr<const Device> device,
              std::shared_ptr<const Allocator> allocator)
        : m_device(device)
        , m_allocator(allocator)
    {
        // Create pipeline, descriptors, etc.
    }

    ImageView& render(CommandBuffer& cmd,
                      const Scene& scene,
                      uint32_t width, uint32_t height,
                      uint32_t frameIndex) {
        // Get or create output image
        auto& colorOutput = get_or_create_image(
            ColorSlot::Color,
            Width{width}, Height{height},
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled,
            frameIndex
        );

        // Set up rendering
        vk::RenderingAttachmentInfo colorAttachment{
            .imageView = colorOutput.handle(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore
        };

        vk::RenderingInfo renderInfo{
            .renderArea = {{0, 0}, {width, height}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        };

        cmd.handle().beginRendering(renderInfo);

        // Draw scene...

        cmd.handle().endRendering();

        return colorOutput;
    }

private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
};
```

## Cache Management

### Clearing Cache

```cpp
// Clear all cached images
pass.clear_cache();

// Typically done on resize or shutdown
void onResize(uint32_t width, uint32_t height) {
    gbufferPass.clear_cache();
    lightingPass.clear_cache();
    tonemappingPass.clear_cache();
}
```

## Best Practices

1. **Use meaningful slot names** for clarity
2. **Cache pipelines** in the pass, not images
3. **Clear cache on resize** to free old images
4. **Use frame indexing** for multi-buffering
5. **Return ImageView references** for chaining passes
