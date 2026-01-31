---
sidebar_position: 1
---

# Allocator

The `Allocator` class provides memory allocation through the Vulkan Memory Allocator (VMA) library.

## Overview

```cpp
#include <VulkanWrapper/Memory/Allocator.h>

auto allocator = AllocatorBuilder()
    .setInstance(instance)
    .setDevice(device)
    .build();
```

## AllocatorBuilder

### Methods

| Method | Description |
|--------|-------------|
| `setInstance(shared_ptr<Instance>)` | Set the Vulkan instance |
| `setDevice(shared_ptr<Device>)` | Set the logical device |
| `build()` | Create the allocator |

### Example

```cpp
auto allocator = AllocatorBuilder()
    .setInstance(instance)
    .setDevice(device)
    .build();
```

## Allocator Class

### Buffer Allocation

Allocate typed buffers with compile-time validation:

```cpp
template <typename BufferType>
std::unique_ptr<BufferType> allocate(std::size_t elementCount);
```

Usage:

```cpp
// Define buffer types
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;
using StagingBuffer = Buffer<Vertex, true, TransferSrcUsage>;

// Allocate buffers
auto vertexBuffer = allocator->allocate<VertexBuffer>(1000);
auto stagingBuffer = allocator->allocate<StagingBuffer>(1000);
```

### Image Allocation

Images are allocated through the ImageBuilder:

```cpp
auto image = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .setExtent(Width{1024}, Height{1024})
    .setUsage(vk::ImageUsageFlagBits::eSampled)
    .build();
```

## Memory Types

VMA automatically selects appropriate memory types:

### Device Local (GPU-only)

```cpp
// GPU-only vertex buffer
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;
auto buffer = allocator->allocate<VertexBuffer>(count);
```

- Fastest GPU access
- Not accessible from CPU
- Use for geometry, textures, etc.

### Host Visible (CPU-accessible)

```cpp
// CPU-writable staging buffer
using StagingBuffer = Buffer<Vertex, true, TransferSrcUsage>;
auto buffer = allocator->allocate<StagingBuffer>(count);

// Write data directly
buffer->write(vertices, 0);
```

- Accessible from CPU
- May be slower for GPU access
- Use for staging, uniform updates

## Allocation Strategies

VMA uses these strategies automatically:

1. **Dedicated allocation** for large resources
2. **Sub-allocation** from memory blocks for smaller resources
3. **Memory type selection** based on usage flags

## Handle Access

```cpp
// Get the VMA allocator handle
VmaAllocator handle = allocator->handle();
```

## Memory Statistics

```cpp
VmaTotalStatistics stats;
vmaCalculateStatistics(allocator->handle(), &stats);

std::cout << "Used memory: " << stats.total.statistics.allocationBytes << '\n';
std::cout << "Allocated blocks: " << stats.total.statistics.blockCount << '\n';
```

## Best Practices

1. **Create one allocator** per device
2. **Reuse allocators** rather than creating new ones
3. **Use appropriate buffer types** (host visible vs device local)
4. **Let VMA choose memory types** - don't override unless necessary
5. **Use staging buffers** for CPU-to-GPU transfers
