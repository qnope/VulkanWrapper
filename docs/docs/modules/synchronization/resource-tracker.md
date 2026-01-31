---
sidebar_position: 2
---

# Resource Tracker

The `ResourceTracker` system automatically manages resource state and generates optimal barriers.

## Overview

```cpp
#include <VulkanWrapper/Synchronization/ResourceTracker.h>

ResourceTracker tracker;

// Track current state
tracker.track(ImageState{
    .image = image->handle(),
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});

// Request new state
tracker.request(ImageState{
    .image = image->handle(),
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});

// Generate barriers
tracker.flush(commandBuffer);
```

## Image State Tracking

### ImageState

```cpp
struct ImageState {
    vk::Image image;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
    vk::ImageSubresourceRange subresourceRange = {
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };
};
```

### Tracking Images

```cpp
// Track initial state (e.g., after transfer)
tracker.track(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});

// Request shader read state
tracker.request(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});

// Flush generates optimal barrier
tracker.flush(cmd);
```

## Buffer State Tracking

### BufferState

```cpp
struct BufferState {
    vk::Buffer buffer;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
    vk::DeviceSize offset = 0;
    vk::DeviceSize size = VK_WHOLE_SIZE;
};
```

### Tracking Buffers

```cpp
// Track after CPU write
tracker.track(BufferState{
    .buffer = stagingBuffer->handle(),
    .stage = vk::PipelineStageFlagBits2::eHost,
    .access = vk::AccessFlagBits2::eHostWrite
});

// Request as vertex buffer
tracker.request(BufferState{
    .buffer = vertexBuffer->handle(),
    .stage = vk::PipelineStageFlagBits2::eVertexInput,
    .access = vk::AccessFlagBits2::eVertexAttributeRead
});
```

## Subresource Tracking

Track individual mip levels or array layers:

```cpp
// Track specific mip level
tracker.track(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite,
    .subresourceRange = {
        vk::ImageAspectFlagBits::eColor,
        0,   // baseMipLevel
        1,   // levelCount
        0,   // baseArrayLayer
        1    // layerCount
    }
});
```

## IntervalSet

Used internally for fine-grained buffer range tracking:

```cpp
// Tracks non-overlapping intervals
IntervalSet<uint64_t> ranges;
ranges.insert({0, 100});
ranges.insert({50, 150});  // Merged with previous
// Result: single interval [0, 150)
```

## Common Patterns

### Texture Upload

```cpp
// After staging write
tracker.track(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// Request transfer dst for copy
tracker.request(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});

tracker.flush(cmd);

// Copy operation...

// Track new state
tracker.track(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});

// Request shader read
tracker.request(ImageState{
    .image = texture->handle(),
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});

tracker.flush(cmd);
```

### G-Buffer Transitions

```cpp
// After geometry pass (written as attachment)
for (auto& gbufferImage : gbuffer) {
    tracker.track(ImageState{
        .image = gbufferImage->handle(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite
    });

    // Request for lighting pass (read as sampled)
    tracker.request(ImageState{
        .image = gbufferImage->handle(),
        .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .stage = vk::PipelineStageFlagBits2::eFragmentShader,
        .access = vk::AccessFlagBits2::eShaderSampledRead
    });
}

tracker.flush(cmd);
```

## Best Practices

1. **Track after modifications** - call `track()` after each operation
2. **Request before use** - call `request()` before needed state
3. **Batch flushes** - group multiple transitions
4. **Use subresource ranges** for partial updates
5. **Let tracker optimize** - it merges compatible barriers
