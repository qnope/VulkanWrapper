---
sidebar_position: 2
---

# Image Views

The `ImageView` class provides a view into an image with specific format and subresource interpretation.

## Overview

```cpp
#include <VulkanWrapper/Image/ImageView.h>

auto view = ImageViewBuilder()
    .setDevice(device)
    .setImage(image)
    .build();
```

## ImageViewBuilder

### Methods

| Method | Description |
|--------|-------------|
| `setDevice(shared_ptr<Device>)` | Set the logical device |
| `setImage(shared_ptr<Image>)` | Set the source image |
| `setViewType(vk::ImageViewType)` | Set view type (2D, cube, array, etc.) |
| `setFormat(vk::Format)` | Override format interpretation |
| `setAspect(vk::ImageAspectFlags)` | Set aspect flags |
| `setBaseMipLevel(uint32_t)` | Set first mip level |
| `setMipLevelCount(uint32_t)` | Set number of mip levels |
| `setBaseArrayLayer(uint32_t)` | Set first array layer |
| `setArrayLayerCount(uint32_t)` | Set number of layers |

### Examples

```cpp
// Basic 2D view (uses image defaults)
auto view = ImageViewBuilder()
    .setDevice(device)
    .setImage(texture)
    .build();

// Specific mip level
auto mipView = ImageViewBuilder()
    .setDevice(device)
    .setImage(texture)
    .setBaseMipLevel(2)
    .setMipLevelCount(1)
    .build();

// Cubemap view
auto cubeView = ImageViewBuilder()
    .setDevice(device)
    .setImage(cubemapImage)
    .setViewType(vk::ImageViewType::eCube)
    .setArrayLayerCount(6)
    .build();

// Depth-only view
auto depthView = ImageViewBuilder()
    .setDevice(device)
    .setImage(depthStencilImage)
    .setAspect(vk::ImageAspectFlagBits::eDepth)
    .build();
```

## ImageView Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::ImageView` | Get the raw handle |
| `image()` | `const Image&` | Get the source image |
| `format()` | `vk::Format` | Get the view format |

## View Types

| Type | Description |
|------|-------------|
| `e1D` | 1D texture |
| `e2D` | 2D texture |
| `e3D` | 3D texture |
| `eCube` | Cubemap (6 layers) |
| `e1DArray` | Array of 1D textures |
| `e2DArray` | Array of 2D textures |
| `eCubeArray` | Array of cubemaps |

## Subresource Ranges

Views can expose a subset of an image:

```cpp
// View only mip levels 2-4
auto view = ImageViewBuilder()
    .setDevice(device)
    .setImage(texture)
    .setBaseMipLevel(2)
    .setMipLevelCount(3)  // Levels 2, 3, 4
    .build();

// View single array layer
auto layerView = ImageViewBuilder()
    .setDevice(device)
    .setImage(arrayTexture)
    .setBaseArrayLayer(5)
    .setArrayLayerCount(1)
    .build();
```

## Format Reinterpretation

Views can interpret the same data as a different format:

```cpp
// Image stored as RGBA8
auto image = ImageBuilder()
    .setFormat(vk::Format::eR8G8B8A8Unorm)
    // ...
    .build();

// View as sRGB
auto srgbView = ImageViewBuilder()
    .setDevice(device)
    .setImage(image)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .build();
```

Formats must be compatible (same size, compatible compression).

## Aspect Flags

For depth-stencil images, views can select specific aspects:

```cpp
// Depth-stencil image
auto dsImage = ImageBuilder()
    .setFormat(vk::Format::eD24UnormS8Uint)
    // ...
    .build();

// Depth-only view (for sampling)
auto depthView = ImageViewBuilder()
    .setDevice(device)
    .setImage(dsImage)
    .setAspect(vk::ImageAspectFlagBits::eDepth)
    .build();

// Stencil-only view
auto stencilView = ImageViewBuilder()
    .setDevice(device)
    .setImage(dsImage)
    .setAspect(vk::ImageAspectFlagBits::eStencil)
    .build();
```

## Use with Descriptors

```cpp
// Bind to descriptor set
vk::DescriptorImageInfo imageInfo{
    .sampler = sampler->handle(),
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};
```

## Use with Attachments

```cpp
// Use as color attachment
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = colorView->handle(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}}
};
```

## Best Practices

1. **Create views as needed** - they're lightweight
2. **Match view type to image** - don't create cube view of 2D image
3. **Use appropriate aspects** for depth-stencil images
4. **Consider format compatibility** when reinterpreting
5. **Cache views** that are used repeatedly
