---
sidebar_position: 3
title: "Samplers"
---

# Samplers

A `Sampler` defines how a shader reads texels from an image -- the filtering mode (nearest vs linear), the address mode (repeat, clamp, mirror), mipmap behavior, and other sampling parameters. Samplers are independent of images; a single sampler can be used with many different image views.

## Design Rationale

In raw Vulkan, creating a sampler requires filling out a `VkSamplerCreateInfo` with 15+ fields, many of which interact in subtle ways (anisotropy requires linear filtering, comparison sampling changes the return type in GLSL, LOD clamping affects mipmap selection). Getting defaults wrong leads to blurry textures, visible seams, or incorrect shadow map results.

VulkanWrapper's `SamplerBuilder` provides sensible defaults and returns an immutable `shared_ptr<const Sampler>`. For the common case -- linear filtering with reasonable defaults -- a single call to `SamplerBuilder(device).build()` is sufficient.

The `CombinedImage` class pairs an `ImageView` with a `Sampler` into a single object, mirroring the `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` descriptor type and providing convenient accessors for descriptor binding.

## API Reference

### SamplerBuilder

```cpp
namespace vw {

class SamplerBuilder {
public:
    SamplerBuilder(std::shared_ptr<const Device> device);

    std::shared_ptr<const Sampler> build();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Image/Sampler.h`

#### Constructor

Takes a `Device` by `shared_ptr`. The `SamplerBuilder` internally initializes a `vk::SamplerCreateInfo` with default values.

#### `build()`

Creates the sampler and returns it as `shared_ptr<const Sampler>`. The sampler is immutable after creation.

### Sampler

```cpp
namespace vw {

class Sampler : public ObjectWithUniqueHandle<vk::UniqueSampler> {
public:
    // Inherits handle() from ObjectWithUniqueHandle
};

} // namespace vw
```

#### `handle()`

Returns the raw `vk::Sampler` handle (inherited from `ObjectWithUniqueHandle`). This is what you pass to `vk::DescriptorImageInfo` when binding textures in descriptor sets.

The sampler is RAII-managed -- it is automatically destroyed when the last `shared_ptr` to it is released.

### CombinedImage

```cpp
namespace vw {

class CombinedImage {
public:
    CombinedImage(std::shared_ptr<const ImageView> image_view,
                  std::shared_ptr<const Sampler> sampler);

    [[nodiscard]] vk::Image image() const noexcept;
    [[nodiscard]] vk::ImageView image_view() const noexcept;
    [[nodiscard]] std::shared_ptr<const ImageView> image_view_ptr() const noexcept;
    [[nodiscard]] vk::Sampler sampler() const noexcept;
    [[nodiscard]] vk::ImageSubresourceRange subresource_range() const noexcept;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Image/CombinedImage.h`

Pairs an `ImageView` and `Sampler` together, keeping both alive through shared ownership. This mirrors the Vulkan concept of a combined image sampler descriptor.

#### Constructor

Takes both the image view and sampler by `shared_ptr`. The `CombinedImage` stores shared pointers to the `Image`, `ImageView`, and `Sampler`, ensuring all three remain alive.

#### `image()`

Returns the raw `vk::Image` handle of the underlying image.

#### `image_view()`

Returns the raw `vk::ImageView` handle. Use this for `vk::DescriptorImageInfo`:

```cpp
vk::DescriptorImageInfo info{
    .sampler = combined.sampler(),
    .imageView = combined.image_view(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};
```

#### `image_view_ptr()`

Returns the `shared_ptr<const ImageView>` for when you need the full VulkanWrapper object rather than just the handle.

#### `sampler()`

Returns the raw `vk::Sampler` handle.

#### `subresource_range()`

Returns the `vk::ImageSubresourceRange` of the underlying image view.

## Sampler Configuration

The `SamplerBuilder` uses `vk::SamplerCreateInfo` internally with defaults from the Vulkan specification. Here is what each setting controls:

### Filtering

Filtering determines how texels are interpolated when the texture is sampled at non-integer coordinates.

| Filter | Behavior |
|--------|----------|
| `vk::Filter::eNearest` | Point sampling -- selects the nearest texel. Produces sharp, pixelated results. |
| `vk::Filter::eLinear` | Bilinear interpolation -- blends the four nearest texels. Produces smooth results. |

The `SamplerCreateInfo` has separate fields for magnification (`magFilter`) and minification (`minFilter`).

### Mipmap Mode

Controls how mip levels are selected when the texture is minified (viewed from far away).

| Mode | Behavior |
|------|----------|
| `vk::SamplerMipmapMode::eNearest` | Selects the closest mip level |
| `vk::SamplerMipmapMode::eLinear` | Interpolates between the two closest mip levels (trilinear filtering when combined with linear min/mag filters) |

### Address Modes

Controls what happens when texture coordinates fall outside the [0, 1] range.

| Mode | Behavior |
|------|----------|
| `vk::SamplerAddressMode::eRepeat` | Tiles the texture infinitely |
| `vk::SamplerAddressMode::eMirroredRepeat` | Tiles with alternating mirroring |
| `vk::SamplerAddressMode::eClampToEdge` | Clamps to the edge texel |
| `vk::SamplerAddressMode::eClampToBorder` | Returns a border color |
| `vk::SamplerAddressMode::eMirrorClampToEdge` | Mirrors once, then clamps |

The create info has separate fields for U, V, and W axes (`addressModeU`, `addressModeV`, `addressModeW`).

### LOD (Level of Detail)

LOD settings control mipmap level selection:

- **`minLod` / `maxLod`:** Clamp the computed LOD to this range.
- **`mipLodBias`:** Added to the computed LOD before clamping (negative values sharpen, positive values blur).

### Anisotropic Filtering

Anisotropic filtering improves texture quality when viewed at steep angles (e.g., floor textures). It is configured via `anisotropyEnable` and `maxAnisotropy` in the create info.

### Comparison Sampling

For shadow mapping, the sampler can compare the sampled depth against a reference value instead of returning the texel directly. Configured via `compareEnable` and `compareOp`.

## Usage Patterns

### Basic Sampler

The simplest case -- a sampler with Vulkan defaults:

```cpp
auto sampler = vw::SamplerBuilder(device).build();
```

### Creating a CombinedImage

Pair an image view with a sampler for descriptor binding:

```cpp
auto image = allocator->create_image_2D(
    vw::Width{512}, vw::Height{512}, true,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

auto view = vw::ImageViewBuilder(device, image).build();
auto sampler = vw::SamplerBuilder(device).build();

vw::CombinedImage combined(view, sampler);
```

### Using CombinedImage from StagingBufferManager

`StagingBufferManager::stage_image_from_path()` returns a `CombinedImage` directly:

```cpp
vw::StagingBufferManager staging(device, allocator);
auto combined = staging.stage_image_from_path("texture.png", true);

auto cmd = staging.fill_command_buffer();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();

// combined is ready for descriptor binding
```

### Binding to a Descriptor Set

```cpp
vk::DescriptorImageInfo imageInfo{
    .sampler = combined.sampler(),
    .imageView = combined.image_view(),
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

### GLSL Shader Usage

In GLSL, a combined image sampler is declared and used like this:

```glsl
layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 color = texture(texSampler, fragTexCoord);
    // 'texture()' uses the sampler's filter, address mode, and LOD settings
}
```

### Sharing Samplers Across Multiple Textures

Samplers are independent of images, so a single sampler can be reused with many textures:

```cpp
auto sampler = vw::SamplerBuilder(device).build();

// Use the same sampler with different textures
vw::CombinedImage diffuse(diffuseView, sampler);
vw::CombinedImage normal(normalView, sampler);
vw::CombinedImage roughness(roughnessView, sampler);
```

This is recommended because Vulkan has a limit on the number of samplers a device can create (typically 4000 for most GPUs).

## Common Pitfalls

### Creating too many samplers

**Symptom:** Vulkan error about exceeding `maxSamplerAllocationCount`.

**Cause:** Each `SamplerBuilder::build()` creates a new GPU sampler object. Vulkan has a device limit on the total number.

**Fix:** Reuse samplers. Most applications need only a handful of sampler configurations (linear/nearest, repeat/clamp, with/without comparison). Create them once and share them across all textures that use the same configuration:

```cpp
auto linearRepeat = vw::SamplerBuilder(device).build();
// Use linearRepeat with all textures that need linear+repeat
```

### Using the wrong image layout in descriptors

**Symptom:** Black textures or validation errors about image layout.

**Cause:** The `imageLayout` in `vk::DescriptorImageInfo` must match the layout the image will be in when the shader reads it. Using `eUndefined` or `eColorAttachmentOptimal` for a sampled texture will fail.

**Fix:** Use `vk::ImageLayout::eShaderReadOnlyOptimal` for textures being sampled:

```cpp
vk::DescriptorImageInfo info{
    .sampler = sampler->handle(),
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};
```

And ensure the image is actually in that layout at the time the shader runs (use `ResourceTracker` for transitions).

### Forgetting to keep CombinedImage alive

**Symptom:** Crash or validation error when the descriptor set is used.

**Cause:** If the `CombinedImage` goes out of scope, the `shared_ptr` reference counts drop. If the image view and sampler are only held by the `CombinedImage`, they are destroyed.

**Fix:** Store `CombinedImage` objects alongside your scene data, ensuring they live as long as the descriptor sets that reference them.

### Using sRGB format with data textures

**Symptom:** Normal maps or other data textures look incorrect.

**Cause:** This is an image format issue, not a sampler issue, but it commonly manifests when setting up samplers and descriptors. sRGB formats apply gamma correction on read.

**Fix:** Use `eR8G8B8A8Unorm` for data textures (normal maps, roughness, metallic) and `eR8G8B8A8Srgb` for color textures (diffuse, albedo). See [Images](images.md) for details.

## Integration with Other Modules

| What | How | See |
|------|-----|-----|
| Create image views | `ImageViewBuilder(device, image)` | [Image Views](image-views.md) |
| Create images | `Allocator::create_image_2D()` | [Images](images.md) |
| Load textures | `StagingBufferManager::stage_image_from_path()` returns `CombinedImage` | [Staging & Transfers](../memory/staging.md) |
| Descriptor binding | `vk::DescriptorImageInfo` with `combined.sampler()` and `combined.image_view()` | Descriptors module |
