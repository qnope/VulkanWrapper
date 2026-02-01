---
sidebar_position: 2
---

# Buffers

VulkanWrapper provides type-safe buffer abstractions with compile-time validation.

## Overview

```cpp
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Memory/BufferUsage.h>

// Define typed buffer
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;

// Allocate
auto buffer = allocator->allocate<VertexBuffer>(1000);
```

## Buffer Template

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);
};
```

### Template Parameters

| Parameter | Description |
|-----------|-------------|
| `T` | Element type stored in buffer |
| `HostVisible` | `true` for CPU-accessible, `false` for GPU-only |
| `Flags` | Buffer usage flags (vertex, index, uniform, etc.) |

## Predefined Usage Flags

```cpp
// Common usage flag combinations
constexpr VkBufferUsageFlags VertexBufferUsage =
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT;

constexpr VkBufferUsageFlags IndexBufferUsage =
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT;

constexpr VkBufferUsageFlags UniformBufferUsage =
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

constexpr VkBufferUsageFlags StorageBufferUsage =
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT;

constexpr VkBufferUsageFlags TransferSrcUsage =
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

constexpr VkBufferUsageFlags TransferDstUsage =
    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
```

## Buffer Type Aliases

Define clear, descriptive buffer types:

```cpp
// Vertex data
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;

// Index data
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

// Uniform data (host visible for updates)
using UniformBuffer = Buffer<UniformData, true, UniformBufferUsage>;

// Staging for transfers
using StagingBuffer = Buffer<std::byte, true, TransferSrcUsage>;

// Storage buffer for compute
using StorageBuffer = Buffer<float, false, StorageBufferUsage>;
```

## Buffer Operations

### Size Information

```cpp
auto buffer = allocator->allocate<VertexBuffer>(1000);

// Element count
std::size_t count = buffer->size();          // 1000

// Byte size
VkDeviceSize bytes = buffer->size_bytes();   // 1000 * sizeof(Vertex)
```

### Writing Data (Host Visible Only)

```cpp
using StagingBuffer = Buffer<Vertex, true, TransferSrcUsage>;
auto staging = allocator->allocate<StagingBuffer>(vertices.size());

// Write span
staging->write(vertices, 0);

// Write single element
staging->write(singleVertex, 5);

// Write raw bytes
staging->write_bytes(rawData, 0);
```

### Reading Data (Host Visible Only)

```cpp
// Read as vector
std::vector<Vertex> data = staging->read_as_vector(0, count);
```

### Device Address

```cpp
// Get GPU address for buffer device address features
vk::DeviceAddress address = buffer->device_address();
```

## Compile-Time Validation

### Usage Validation

```cpp
// Check if buffer supports a usage
static_assert(VertexBuffer::does_support(
    vk::BufferUsageFlagBits::eVertexBuffer));

static_assert(!VertexBuffer::does_support(
    vk::BufferUsageFlagBits::eUniformBuffer));  // False
```

### Host Visibility

```cpp
// Write only available for host visible buffers
using GPUBuffer = Buffer<Vertex, false, VertexBufferUsage>;
using CPUBuffer = Buffer<Vertex, true, TransferSrcUsage>;

CPUBuffer cpu = /* ... */;
cpu.write(data, 0);  // OK

GPUBuffer gpu = /* ... */;
gpu.write(data, 0);  // Compile error!
```

## BufferList

Dynamic buffer allocation with automatic paging:

```cpp
#include <VulkanWrapper/Memory/BufferList.h>

BufferList<Vertex, VertexBufferUsage> bufferList(allocator);

// Add data (automatically allocates new pages as needed)
auto [buffer, offset] = bufferList.push(vertices);

// Use the buffer
cmd.bindVertexBuffers(0, {buffer->handle()}, {offset});
```

### Page Size

BufferList uses 16MB pages by default, allocating new pages as needed.

## StagingBufferManager

Batch CPU-to-GPU transfers:

```cpp
#include <VulkanWrapper/Memory/StagingBufferManager.h>

StagingBufferManager staging(allocator);

// Stage multiple transfers
staging.stage(vertexBuffer, vertexData);
staging.stage(indexBuffer, indexData);
staging.stage(image, imageData);

// Execute all transfers
staging.flush(commandBuffer);
```

## UniformBufferAllocator

Aligned uniform buffer allocation:

```cpp
#include <VulkanWrapper/Memory/UniformBufferAllocator.h>

UniformBufferAllocator uniformAllocator(allocator, device);

// Allocate aligned uniform data
auto [buffer, offset] = uniformAllocator.allocate<UniformData>();

// Write data
buffer->write(uniformData, offset);

// Bind with dynamic offset
cmd.bindDescriptorSets(bindPoint, layout, 0, descriptorSet,
                       {static_cast<uint32_t>(offset)});
```

## Best Practices

1. **Define type aliases** for clarity
2. **Use staging buffers** for GPU-only buffer data
3. **Use `BufferList`** for dynamic geometry
4. **Use `StagingBufferManager`** for batched transfers
5. **Check `does_support()`** at compile time
6. **Prefer GPU-only buffers** for performance
