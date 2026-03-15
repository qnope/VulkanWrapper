---
sidebar_position: 1
title: "Allocator"
---

# Allocator

The `Allocator` is the central memory management object in VulkanWrapper. It wraps the [Vulkan Memory Allocator (VMA)](https://gpuopen.com/vulkan-memory-allocator/) library to provide automatic memory type selection, sub-allocation from large memory blocks, and RAII-based cleanup. All GPU memory allocations -- images and buffers -- flow through the `Allocator`.

## Design Rationale

Raw Vulkan memory management is notoriously complex. You must enumerate memory types, choose between device-local and host-visible heaps, handle alignment requirements, and decide between dedicated allocations and sub-allocations from larger blocks. Getting this wrong leads to poor performance (wrong memory type), wasted memory (too many small allocations), or outright failures (exceeding allocation limits).

VMA handles all of this automatically. VulkanWrapper's `Allocator` wraps VMA and integrates it with the library's RAII model:

- Buffers allocated through the `Allocator` automatically free their VMA allocation on destruction.
- Images created through the `Allocator` use `shared_ptr` with a custom deleter that frees the VMA allocation, so the `Image` class itself has no VMA dependency.
- The `Allocator` uses `enable_shared_from_this` to ensure it outlives all objects allocated from it.

## API Reference

### AllocatorBuilder

```cpp
namespace vw {

class AllocatorBuilder {
public:
    AllocatorBuilder(std::shared_ptr<const Instance> instance,
                     std::shared_ptr<const Device> device);

    std::shared_ptr<Allocator> build();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/Allocator.h`

The builder takes both the `Instance` and `Device` by shared pointer -- VMA needs both to initialize. No additional configuration is necessary; VMA uses the instance and device to determine available memory types and heaps.

#### `build()`

Creates the VMA allocator and returns it wrapped in a `shared_ptr<Allocator>`. Throws `VMAException` if initialization fails.

### Allocator

```cpp
namespace vw {

class Allocator : public std::enable_shared_from_this<Allocator> {
public:
    [[nodiscard]] VmaAllocator handle() const noexcept;

    [[nodiscard]] std::shared_ptr<const Image>
    create_image_2D(Width width, Height height, bool mipmap,
                    vk::Format format, vk::ImageUsageFlags usage) const;

    [[nodiscard]] IndexBuffer allocate_index_buffer(VkDeviceSize size) const;

    [[nodiscard]] BufferBase
    allocate_buffer(VkDeviceSize size, bool host_visible,
                    vk::BufferUsageFlags usage,
                    vk::SharingMode sharing_mode) const;
};

} // namespace vw
```

`Allocator` is non-copyable and non-movable. It is always managed through a `shared_ptr`.

#### `handle()`

Returns the underlying `VmaAllocator` handle. You may need this for advanced VMA operations (statistics, defragmentation) that VulkanWrapper does not wrap.

#### `create_image_2D()`

Creates a 2D image with GPU-local memory, returning a `shared_ptr<const Image>`. Parameters:

| Parameter | Type | Description |
|-----------|------|-------------|
| `width` | `Width` | Image width in pixels |
| `height` | `Height` | Image height in pixels |
| `mipmap` | `bool` | If `true`, calculates and allocates the full mip chain |
| `format` | `vk::Format` | Pixel format (e.g., `vk::Format::eR8G8B8A8Srgb`) |
| `usage` | `vk::ImageUsageFlags` | How the image will be used |

The returned `Image` has no direct VMA dependency -- cleanup happens through the `shared_ptr`'s custom deleter, which frees the VMA allocation. This design allows the `Image` class to live in the `vw.vulkan` module without pulling in VMA.

```cpp
auto image = allocator->create_image_2D(
    vw::Width{1024}, vw::Height{1024},
    true,  // generate mip chain
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);
```

#### `allocate_index_buffer()`

Convenience method that allocates a device-local index buffer (`Buffer<uint32_t, false, IndexBufferUsage>`). The `size` parameter is the buffer size **in bytes**, not the number of indices.

```cpp
auto indexBuffer = allocator->allocate_index_buffer(
    indexCount * sizeof(uint32_t));
```

#### `allocate_buffer()`

Low-level buffer allocation returning a `BufferBase`. This is the method that all higher-level buffer creation functions call internally. Most code should use `create_buffer<>()` or `allocate_vertex_buffer<>()` from `AllocateBufferUtils.h` instead -- see [Buffers](buffers.md).

Parameters:

| Parameter | Type | Description |
|-----------|------|-------------|
| `size` | `VkDeviceSize` | Size in bytes |
| `host_visible` | `bool` | `true` for CPU-accessible memory |
| `usage` | `vk::BufferUsageFlags` | Buffer usage flags |
| `sharing_mode` | `vk::SharingMode` | Sharing mode (typically `eExclusive`) |

## Usage Patterns

### Basic Setup

The allocator is typically created once right after the instance and device:

```cpp
auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .addPortability()
    .build();

auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

auto allocator = vw::AllocatorBuilder(instance, device).build();
```

### Creating Images

The most common use of the `Allocator` is creating images for textures, render targets, and depth buffers:

```cpp
// Color render target (HDR)
auto colorTarget = allocator->create_image_2D(
    vw::Width{1920}, vw::Height{1080},
    false,  // no mipmaps for render targets
    vk::Format::eR16G16B16A16Sfloat,
    vk::ImageUsageFlagBits::eColorAttachment |
    vk::ImageUsageFlagBits::eSampled);

// Depth buffer
auto depthBuffer = allocator->create_image_2D(
    vw::Width{1920}, vw::Height{1080},
    false,
    vk::Format::eD32Sfloat,
    vk::ImageUsageFlagBits::eDepthStencilAttachment);

// Texture with mipmaps
auto texture = allocator->create_image_2D(
    vw::Width{1024}, vw::Height{1024},
    true,  // auto-calculate mip levels
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);
```

### Creating Typed Buffers

While `allocate_buffer()` exists for low-level use, prefer the typed helpers from `AllocateBufferUtils.h`:

```cpp
// Typed vertex buffer (device-local)
auto vertexBuf = vw::allocate_vertex_buffer<Vertex, false>(*allocator, 1000);

// Typed uniform buffer (host-visible)
auto uniformBuf = vw::create_buffer<UniformData, true, vw::UniformBufferUsage>(
    *allocator, 1);

// Typed storage buffer (device-local)
auto storageBuf = vw::create_buffer<float, false, vw::StorageBufferUsage>(
    *allocator, 4096);
```

See [Buffers](buffers.md) for a full explanation of the buffer type system.

### VMA Statistics (Advanced)

For debugging memory usage:

```cpp
VmaTotalStatistics stats;
vmaCalculateStatistics(allocator->handle(), &stats);

// stats.total.statistics.allocationBytes -- total bytes allocated
// stats.total.statistics.blockCount -- number of memory blocks
```

## Memory Types Explained

VMA automatically selects the best memory type, but understanding the tradeoffs helps:

### Device Local (GPU-only)

Used when `host_visible` is `false` (or for images, which are always device-local). This is the fastest memory for GPU operations but cannot be read or written by the CPU. Data must be uploaded via staging buffers and transfer commands.

Best for: vertex buffers, index buffers, textures, render targets, storage buffers used only by shaders.

### Host Visible (CPU-accessible)

Used when `host_visible` is `true`. The CPU can read and write this memory directly via `write()` / `read_as_vector()`. The GPU can also access it, though potentially slower than device-local memory depending on the hardware.

Best for: uniform buffers updated every frame, staging buffers for uploads, readback buffers for screenshots.

## Common Pitfalls

### Creating multiple allocators

**Symptom:** Excessive memory consumption, inability to share memory blocks between resources.

**Cause:** Each `Allocator` creates its own VMA instance with separate memory pools.

**Fix:** Create exactly one `Allocator` per device and share it everywhere. The `shared_ptr`-based ownership model makes this easy.

### Using `allocate_buffer()` when `create_buffer<>()` is available

**Symptom:** Loss of compile-time type safety, manual size calculations, raw `BufferBase` handles.

**Cause:** `allocate_buffer()` is the low-level escape hatch. It returns an untyped `BufferBase` and takes size in bytes.

**Fix:** Prefer the typed helpers:

```cpp
// Instead of:
auto buf = allocator->allocate_buffer(
    100 * sizeof(float), true,
    vk::BufferUsageFlags(vw::UniformBufferUsage),
    vk::SharingMode::eExclusive);

// Use:
auto buf = vw::create_buffer<float, true, vw::UniformBufferUsage>(
    *allocator, 100);
```

### Forgetting `eTransferDst` on images that will receive uploads

**Symptom:** Validation error when copying data to the image via a staging buffer.

**Cause:** An image must have `eTransferDst` in its usage flags to be the target of a copy or blit operation.

**Fix:** Include `eTransferDst` when creating images that will be filled with data:

```cpp
auto image = allocator->create_image_2D(
    width, height, true, format,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst);
```

### Forgetting `eTransferSrc` on images that need mipmaps

**Symptom:** Validation error during mipmap generation.

**Cause:** Mipmap generation works by blitting from one mip level to the next. The image must support being a blit source.

**Fix:** Include both `eTransferSrc` and `eTransferDst`:

```cpp
auto image = allocator->create_image_2D(
    width, height, true, format,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);
```

## Integration with Other Modules

| What | How | See |
|------|-----|-----|
| Create typed buffers | `create_buffer<T, HostVisible, Usage>(*allocator, count)` | [Buffers](buffers.md) |
| Create vertex buffers | `allocate_vertex_buffer<V, false>(*allocator, count)` | [Buffers](buffers.md) |
| Upload data to GPU | `StagingBufferManager(device, allocator)` | [Staging & Transfers](staging.md) |
| Create image views | `ImageViewBuilder(device, image)` | [Image Views](../image/image-views.md) |
| Save GPU image to disk | `transfer.saveToFile(cmd, *allocator, queue, image, "out.png")` | [Staging & Transfers](staging.md) |
