---
sidebar_position: 3
---

# Samplers

The `Sampler` class defines how textures are sampled in shaders.

## Overview

```cpp
#include <VulkanWrapper/Image/Sampler.h>

auto sampler = SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eLinear)
    .setAddressMode(vk::SamplerAddressMode::eRepeat)
    .build();
```

## SamplerBuilder

### Filter Settings

| Method | Description |
|--------|-------------|
| `setFilter(vk::Filter)` | Set min and mag filter |
| `setMinFilter(vk::Filter)` | Set minification filter |
| `setMagFilter(vk::Filter)` | Set magnification filter |
| `setMipmapMode(vk::SamplerMipmapMode)` | Set mipmap interpolation |

### Address Modes

| Method | Description |
|--------|-------------|
| `setAddressMode(vk::SamplerAddressMode)` | Set all axes |
| `setAddressModeU(vk::SamplerAddressMode)` | Set U axis |
| `setAddressModeV(vk::SamplerAddressMode)` | Set V axis |
| `setAddressModeW(vk::SamplerAddressMode)` | Set W axis |

### LOD Settings

| Method | Description |
|--------|-------------|
| `setMinLod(float)` | Minimum LOD level |
| `setMaxLod(float)` | Maximum LOD level |
| `setMipLodBias(float)` | LOD bias |

### Other Settings

| Method | Description |
|--------|-------------|
| `setMaxAnisotropy(float)` | Anisotropic filtering level |
| `setCompareOp(vk::CompareOp)` | Enable comparison sampling |
| `setBorderColor(vk::BorderColor)` | Border color for clamp mode |

### Examples

```cpp
// Linear filtering with repeat
auto defaultSampler = SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressMode(vk::SamplerAddressMode::eRepeat)
    .setMaxLod(VK_LOD_CLAMP_NONE)
    .build();

// Anisotropic filtering
auto anisotropicSampler = SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eLinear)
    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
    .setAddressMode(vk::SamplerAddressMode::eRepeat)
    .setMaxAnisotropy(16.0f)
    .setMaxLod(VK_LOD_CLAMP_NONE)
    .build();

// Nearest neighbor (pixel art)
auto pixelSampler = SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eNearest)
    .setMipmapMode(vk::SamplerMipmapMode::eNearest)
    .setAddressMode(vk::SamplerAddressMode::eClampToEdge)
    .build();

// Shadow map sampler
auto shadowSampler = SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eLinear)
    .setAddressMode(vk::SamplerAddressMode::eClampToBorder)
    .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
    .setCompareOp(vk::CompareOp::eLessOrEqual)
    .build();
```

## Filter Modes

### Magnification/Minification

| Filter | Description |
|--------|-------------|
| `eNearest` | Point sampling, no interpolation |
| `eLinear` | Bilinear interpolation |

### Mipmap Mode

| Mode | Description |
|------|-------------|
| `eNearest` | Use nearest mip level |
| `eLinear` | Interpolate between mip levels (trilinear) |

## Address Modes

| Mode | Description |
|------|-------------|
| `eRepeat` | Tile the texture |
| `eMirroredRepeat` | Tile with mirroring |
| `eClampToEdge` | Clamp to edge pixels |
| `eClampToBorder` | Use border color outside |
| `eMirrorClampToEdge` | Mirror once, then clamp |

## CombinedImage

Convenience wrapper for ImageView + Sampler:

```cpp
#include <VulkanWrapper/Image/CombinedImage.h>

// Create combined image for easy descriptor binding
CombinedImage combined(imageView, sampler);

// Get descriptor info
vk::DescriptorImageInfo info = combined.descriptor_info(
    vk::ImageLayout::eShaderReadOnlyOptimal
);
```

## Default Samplers

Screen-space passes often use a default sampler:

```cpp
// Create in ScreenSpacePass
auto sampler = create_default_sampler(device);
// Linear filter, clamp to edge, no mipmaps
```

## Use in Shaders

GLSL:
```glsl
layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, texCoord);
}
```

## Use with Descriptors

```cpp
// Bind to descriptor set
vk::DescriptorImageInfo imageInfo{
    .sampler = sampler->handle(),
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};

vk::WriteDescriptorSet write{
    .dstSet = descriptorSet,
    .dstBinding = 0,
    .descriptorCount = 1,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .pImageInfo = &imageInfo
};

device->handle().updateDescriptorSets(write, {});
```

## Best Practices

1. **Reuse samplers** - they're expensive to create
2. **Use anisotropic filtering** for textures viewed at angles
3. **Match sampler to use case** - shadow maps need comparison
4. **Consider mipmaps** for textures at varying distances
5. **Use `eClampToEdge`** for screen-space effects
