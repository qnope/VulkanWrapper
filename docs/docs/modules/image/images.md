---
sidebar_position: 1
---

# Images

The `Image` class represents Vulkan images (textures) with automatic memory management.

## Overview

```cpp
#include <VulkanWrapper/Image/Image.h>

auto image = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .setExtent(Width{1024}, Height{1024})
    .setUsage(vk::ImageUsageFlagBits::eSampled |
              vk::ImageUsageFlagBits::eTransferDst)
    .build();
```

## ImageBuilder

### Required Methods

| Method | Description |
|--------|-------------|
| `setDevice(shared_ptr<Device>)` | Set the logical device |
| `setAllocator(shared_ptr<Allocator>)` | Set the memory allocator |
| `setFormat(vk::Format)` | Set the image format |
| `setExtent(Width, Height)` | Set 2D dimensions |
| `setUsage(vk::ImageUsageFlags)` | Set usage flags |

### Optional Methods

| Method | Description |
|--------|-------------|
| `setDepth(Depth)` | Set depth for 3D images |
| `setMipLevels(uint32_t)` | Set mipmap levels |
| `setArrayLayers(uint32_t)` | Set array layers |
| `setImageType(vk::ImageType)` | Set image type (1D, 2D, 3D) |
| `setTiling(vk::ImageTiling)` | Set tiling mode |
| `setSamples(vk::SampleCountFlagBits)` | Set MSAA samples |

### Examples

```cpp
// Color attachment
auto colorImage = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR16G16B16A16Sfloat)
    .setExtent(Width{1920}, Height{1080})
    .setUsage(vk::ImageUsageFlagBits::eColorAttachment |
              vk::ImageUsageFlagBits::eSampled)
    .build();

// Depth buffer
auto depthImage = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eD32Sfloat)
    .setExtent(Width{1920}, Height{1080})
    .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
    .build();

// Texture with mipmaps
auto texture = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .setExtent(Width{1024}, Height{1024})
    .setMipLevels(11)  // log2(1024) + 1
    .setUsage(vk::ImageUsageFlagBits::eSampled |
              vk::ImageUsageFlagBits::eTransferDst |
              vk::ImageUsageFlagBits::eTransferSrc)
    .build();

// Cubemap
auto cubemap = ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR16G16B16A16Sfloat)
    .setExtent(Width{512}, Height{512})
    .setArrayLayers(6)
    .setUsage(vk::ImageUsageFlagBits::eSampled |
              vk::ImageUsageFlagBits::eTransferDst)
    .build();
```

## Image Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::Image` | Get the raw handle |
| `format()` | `vk::Format` | Get the format |
| `extent()` | `vk::Extent3D` | Get dimensions |
| `mip_levels()` | `uint32_t` | Get mip level count |
| `array_layers()` | `uint32_t` | Get array layer count |
| `aspect()` | `vk::ImageAspectFlags` | Get aspect flags |

### Aspect Flags

Automatically determined from format:

```cpp
// Color format -> eColor
// Depth format -> eDepth
// Depth-stencil -> eDepth | eStencil
vk::ImageAspectFlags aspect = image->aspect();
```

## Image Formats

### Common Color Formats

| Format | Description |
|--------|-------------|
| `eR8G8B8A8Srgb` | 8-bit RGBA, sRGB |
| `eR8G8B8A8Unorm` | 8-bit RGBA, linear |
| `eR16G16B16A16Sfloat` | 16-bit float RGBA (HDR) |
| `eR32G32B32A32Sfloat` | 32-bit float RGBA |

### Depth Formats

| Format | Description |
|--------|-------------|
| `eD32Sfloat` | 32-bit float depth |
| `eD24UnormS8Uint` | 24-bit depth + 8-bit stencil |
| `eD32SfloatS8Uint` | 32-bit depth + 8-bit stencil |

## Loading Images

Use `ImageLoader` to load from disk:

```cpp
#include <VulkanWrapper/Image/ImageLoader.h>

// Load with automatic format detection
auto image = ImageLoader::load(
    device, allocator, commandBuffer,
    "texture.png"
);

// Load with specific format
auto image = ImageLoader::load(
    device, allocator, commandBuffer,
    "texture.png",
    vk::Format::eR8G8B8A8Srgb
);
```

## Mipmap Generation

```cpp
#include <VulkanWrapper/Image/Mipmap.h>

// Generate mipmaps (image must have TransferSrc usage)
vw::generateMipmaps(commandBuffer, image);
```

The image must be in the correct layout before generation:
- Mip level 0: `eTransferSrcOptimal`
- Other mips: `eUndefined`

After generation, all mips are in `eTransferSrcOptimal`.

## Image Usage Flags

| Flag | Description |
|------|-------------|
| `eSampled` | Can be sampled in shaders |
| `eStorage` | Can be used as storage image |
| `eColorAttachment` | Can be color attachment |
| `eDepthStencilAttachment` | Can be depth/stencil attachment |
| `eTransferSrc` | Can be copy source |
| `eTransferDst` | Can be copy destination |
| `eInputAttachment` | Can be input attachment |

## Best Practices

1. **Choose appropriate formats** for quality vs. memory
2. **Use mipmaps** for textures viewed at varying distances
3. **Combine usage flags** as needed
4. **Use `eOptimal` tiling** for GPU performance
5. **Consider memory budget** for high-resolution textures
