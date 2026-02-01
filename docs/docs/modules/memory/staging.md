---
sidebar_position: 3
---

# Staging and Transfers

VulkanWrapper provides utilities for efficient CPU-to-GPU data transfers.

## Overview

GPU-optimal memory isn't directly accessible from the CPU. To upload data:

1. Write data to a host-visible staging buffer
2. Issue a transfer command to copy to GPU memory
3. Synchronize before using the data

## StagingBufferManager

Batches multiple transfers for efficiency:

```cpp
#include <VulkanWrapper/Memory/StagingBufferManager.h>

StagingBufferManager staging(allocator);
```

### Staging Buffer Data

```cpp
// Stage vertex data
staging.stage(vertexBuffer, vertices);

// Stage index data
staging.stage(indexBuffer, indices);

// Stage with offset
staging.stage(buffer, data, offsetInElements);
```

### Staging Image Data

```cpp
// Stage entire image
staging.stage(image, imageData);

// Stage specific mip level
staging.stage(image, mipData, MipLevel{1});

// Stage specific layer
staging.stage(image, layerData, ArrayLayer{0}, MipLevel{0});
```

### Flushing Transfers

```cpp
// Execute all staged transfers
staging.flush(commandBuffer);

// The command buffer must be submitted and completed
// before using the transferred data
```

## Transfer Module

Low-level transfer functions:

```cpp
#include <VulkanWrapper/Memory/Transfer.h>
```

### Buffer-to-Buffer Copy

```cpp
vw::transfer(commandBuffer,
             sourceBuffer,      // Source
             destinationBuffer, // Destination
             size,              // Size in bytes
             srcOffset,         // Source offset
             dstOffset);        // Destination offset
```

### Buffer-to-Image Copy

```cpp
vw::transfer(commandBuffer,
             stagingBuffer,
             image,
             Width{1024},
             Height{1024},
             MipLevel{0});
```

## Manual Staging

For fine-grained control:

```cpp
// 1. Create staging buffer
using Staging = Buffer<Vertex, true, TransferSrcUsage>;
auto staging = allocator->allocate<Staging>(vertices.size());

// 2. Write data
staging->write(vertices, 0);

// 3. Record copy command
vk::BufferCopy copyRegion{
    .srcOffset = 0,
    .dstOffset = 0,
    .size = staging->size_bytes()
};

commandBuffer.copyBuffer(
    staging->handle(),
    vertexBuffer->handle(),
    copyRegion
);

// 4. Add barrier before using
vk::BufferMemoryBarrier2 barrier{
    .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
    .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eVertexInput,
    .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead,
    .buffer = vertexBuffer->handle(),
    .size = VK_WHOLE_SIZE
};

vk::DependencyInfo depInfo{
    .bufferMemoryBarrierCount = 1,
    .pBufferMemoryBarriers = &barrier
};

commandBuffer.pipelineBarrier2(depInfo);
```

## Image Transfers

### Uploading Textures

```cpp
// Load image data
auto imageData = loadImageFromDisk("texture.png");

// Create image
auto image = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .setExtent(Width{imageData.width}, Height{imageData.height})
    .setUsage(vk::ImageUsageFlagBits::eSampled |
              vk::ImageUsageFlagBits::eTransferDst)
    .build();

// Stage and transfer
StagingBufferManager staging(allocator);
staging.stage(image, imageData.pixels);
staging.flush(commandBuffer);
```

### Mipmap Generation

```cpp
#include <VulkanWrapper/Image/Mipmap.h>

// Generate mipmaps after uploading base level
vw::generateMipmaps(commandBuffer, image);
```

## ResourceTracker Integration

Use `ResourceTracker` for automatic barrier generation:

```cpp
ResourceTracker tracker;

// Track current state
tracker.track(BufferState{
    .buffer = stagingBuffer->handle(),
    .stage = vk::PipelineStageFlagBits2::eHost,
    .access = vk::AccessFlagBits2::eHostWrite
});

// Request new state after transfer
tracker.request(BufferState{
    .buffer = vertexBuffer->handle(),
    .stage = vk::PipelineStageFlagBits2::eVertexInput,
    .access = vk::AccessFlagBits2::eVertexAttributeRead
});

// Generate barriers
tracker.flush(commandBuffer);
```

## Best Practices

1. **Batch transfers** using `StagingBufferManager`
2. **Reuse staging buffers** when possible
3. **Use transfer queues** for async transfers if available
4. **Generate mipmaps** on GPU, not CPU
5. **Synchronize properly** before using transferred data
6. **Consider memory mapping** for frequently updated data
