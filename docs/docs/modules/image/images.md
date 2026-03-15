---
sidebar_position: 1
title: "Images"
---

# Images

The `Image` class represents a Vulkan image -- the GPU-side resource that stores textures, render targets, depth buffers, and other 2D/3D pixel data. Images are created through the `Allocator` and their memory is managed automatically via `shared_ptr` with a custom deleter.

## Design Rationale

In raw Vulkan, creating an image requires creating the `VkImage` object, allocating memory for it, and binding the memory to the image -- three separate API calls with many opportunities for mismatched parameters. The `Image` must also be tracked for layout transitions, and its VMA allocation must be freed when it is destroyed.

VulkanWrapper simplifies this to a single call: `Allocator::create_image_2D()`. The allocator handles VMA allocation and binding, and wraps the result in a `shared_ptr<const Image>` whose custom deleter frees the VMA allocation automatically. This design has an important architectural benefit: the `Image` class itself has **no VMA dependency**. It lives in the `vw.vulkan` module and stores only the `vk::Image` handle and metadata (format, extent, mip levels). The VMA cleanup is encapsulated entirely in the `shared_ptr`'s deleter, set up by the `Allocator`.

Images are always immutable after creation -- their format, extent, and mip level count are fixed. You interact with them through `ImageView` objects (see [Image Views](image-views.md)) and layout transitions managed by the `ResourceTracker`.

## API Reference

### Image

```cpp
namespace vw {

class Image : public ObjectWithHandle<vk::Image> {
public:
    [[nodiscard]] MipLevel mip_levels() const noexcept;
    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] vk::ImageSubresourceRange full_range() const noexcept;
    [[nodiscard]] vk::ImageSubresourceRange
    mip_level_range(MipLevel mip_level) const noexcept;
    [[nodiscard]] vk::ImageSubresourceLayers
    mip_level_layer(MipLevel mip_level) const noexcept;

    [[nodiscard]] vk::Extent2D extent2D() const noexcept;
    [[nodiscard]] vk::Extent3D extent3D() const noexcept;
    [[nodiscard]] vk::Extent3D mip_level_extent3D(MipLevel mip_level) const;

    [[nodiscard]] std::array<vk::Offset3D, 2>
    mip_level_offsets(MipLevel mip_level) const noexcept;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Image/Image.h`

You do not construct `Image` directly. Use `Allocator::create_image_2D()` to create images.

#### `handle()`

Returns the raw `vk::Image` handle (inherited from `ObjectWithHandle<vk::Image>`). Needed for Vulkan API calls like `cmd.beginRendering()`.

#### `mip_levels()`

Returns the number of mip levels as a `MipLevel` strong type. If the image was created with `mipmap = true`, this is `floor(log2(max(width, height))) + 1`. Otherwise it is `MipLevel{1}`.

#### `format()`

Returns the pixel format (`vk::Format`). This is the format specified at creation time and does not change.

#### `full_range()`

Returns a `vk::ImageSubresourceRange` covering all mip levels and array layers of the image. This is the range you typically use for layout transitions and barriers:

```cpp
auto range = image->full_range();
// range.aspectMask = eColor (for color images)
// range.baseMipLevel = 0
// range.levelCount = image->mip_levels()
// range.baseArrayLayer = 0
// range.layerCount = 1
```

#### `mip_level_range(MipLevel mip_level)`

Returns a subresource range covering a single mip level. Useful for per-level operations like mipmap generation:

```cpp
auto range = image->mip_level_range(MipLevel{2});
// Covers only mip level 2
```

#### `mip_level_layer(MipLevel mip_level)`

Returns a `vk::ImageSubresourceLayers` for a single mip level. This is the type used by copy and blit commands.

#### `extent2D()` / `extent3D()`

Return the base-level image dimensions:

```cpp
auto ext = image->extent2D();
// ext.width, ext.height

auto ext3 = image->extent3D();
// ext3.width, ext3.height, ext3.depth
```

#### `mip_level_extent3D(MipLevel mip_level)`

Returns the dimensions of a specific mip level (each successive level halves each dimension, clamped to 1):

```cpp
// For a 1024x1024 image:
image->mip_level_extent3D(MipLevel{0});  // {1024, 1024, 1}
image->mip_level_extent3D(MipLevel{1});  // {512, 512, 1}
image->mip_level_extent3D(MipLevel{2});  // {256, 256, 1}
```

#### `mip_level_offsets(MipLevel mip_level)`

Returns a pair of `vk::Offset3D` representing the blit region for a mip level: `{0, 0, 0}` and `{width, height, depth}`. Used internally by mipmap generation and blit operations.

### Image Creation via Allocator

```cpp
std::shared_ptr<const Image>
Allocator::create_image_2D(Width width, Height height, bool mipmap,
                           vk::Format format, vk::ImageUsageFlags usage) const;
```

**Header:** `VulkanWrapper/Memory/Allocator.h`

See [Allocator](../memory/allocator.md) for full details. Quick reference:

| Parameter | Description |
|-----------|-------------|
| `width` | Image width in pixels (`Width` strong type) |
| `height` | Image height in pixels (`Height` strong type) |
| `mipmap` | `true` to allocate full mip chain, `false` for single level |
| `format` | Pixel format |
| `usage` | Combination of `vk::ImageUsageFlagBits` values |

### ImageLoader

```cpp
namespace vw {

struct ImageDescription {
    Width width;
    Height height;
    std::vector<std::byte> pixels;
};

ImageDescription load_image(const std::filesystem::path &path);

void save_image(const std::filesystem::path &path, Width width, Height height,
                std::span<const std::byte> pixels);

} // namespace vw
```

**Header:** `VulkanWrapper/Image/ImageLoader.h`

Low-level image file I/O using SDL_image.

#### `load_image(path)`

Loads an image from disk and returns an `ImageDescription` containing the dimensions and raw pixel data. The pixel data is in RGBA8 format (4 bytes per pixel). Supports any format SDL_image can read (PNG, JPG, BMP, TGA, etc.).

```cpp
auto desc = vw::load_image("textures/diffuse.png");
// desc.width, desc.height, desc.pixels
```

#### `save_image(path, width, height, pixels)`

Writes RGBA8 pixel data to an image file. The format is determined by the file extension (`.png`, `.bmp`, `.jpg`).

### Mipmap Generation

```cpp
namespace vw {

void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image);

} // namespace vw
```

**Header:** `VulkanWrapper/Image/Mipmap.h`

Generates a full mip chain from the base level by performing successive blit operations from each level to the next. The image must:

- Have been created with `mipmap = true` (to allocate space for the chain).
- Include `eTransferSrc` and `eTransferDst` in its usage flags (because each level is both a source and a destination during generation).

```cpp
// After uploading base level data to the image:
vw::generate_mipmap(cmd, textureImage);
```

## Image Formats

### Common Color Formats

| Format | Bits/Pixel | Description |
|--------|-----------|-------------|
| `eR8G8B8A8Srgb` | 32 | 8-bit RGBA with sRGB gamma (standard for diffuse textures) |
| `eR8G8B8A8Unorm` | 32 | 8-bit RGBA, linear (for data textures like normal maps) |
| `eR16G16B16A16Sfloat` | 64 | 16-bit float RGBA (HDR render targets) |
| `eR32G32B32A32Sfloat` | 128 | 32-bit float RGBA (high-precision compute) |
| `eB8G8R8A8Srgb` | 32 | 8-bit BGRA with sRGB (common swapchain format) |

### Depth Formats

| Format | Description |
|--------|-------------|
| `eD32Sfloat` | 32-bit float depth (highest precision) |
| `eD24UnormS8Uint` | 24-bit depth + 8-bit stencil |
| `eD32SfloatS8Uint` | 32-bit float depth + 8-bit stencil |

### Choosing a Format

- **Diffuse/albedo textures:** `eR8G8B8A8Srgb` -- the sRGB encoding matches how artists create textures.
- **Normal maps, data textures:** `eR8G8B8A8Unorm` -- linear data should not have sRGB gamma applied.
- **HDR render targets:** `eR16G16B16A16Sfloat` -- 16-bit float provides enough range for HDR lighting.
- **Depth buffers:** `eD32Sfloat` -- 32-bit float depth gives the best precision for typical scenes.

## Image Usage Flags

Usage flags tell Vulkan how the image will be used. You must specify all intended uses at creation time.

| Flag | Description |
|------|-------------|
| `eSampled` | Can be sampled in shaders (textures) |
| `eStorage` | Can be used as a storage image (read/write in compute shaders) |
| `eColorAttachment` | Can be a color render target |
| `eDepthStencilAttachment` | Can be a depth or stencil render target |
| `eTransferSrc` | Can be a source for copy/blit operations |
| `eTransferDst` | Can be a destination for copy/blit operations |
| `eInputAttachment` | Can be an input attachment in a render pass |

Common combinations:

```cpp
// Texture uploaded from CPU
vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst

// Texture with mipmaps
vk::ImageUsageFlagBits::eSampled |
vk::ImageUsageFlagBits::eTransferDst |
vk::ImageUsageFlagBits::eTransferSrc

// Color render target that will also be sampled
vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled

// Depth buffer
vk::ImageUsageFlagBits::eDepthStencilAttachment
```

## Usage Patterns

### Creating a Texture with Mipmaps

```cpp
// Create the image
auto texture = allocator->create_image_2D(
    vw::Width{1024}, vw::Height{1024},
    true,  // allocate full mip chain
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);

// Upload base level via StagingBufferManager...

// Generate mipmaps on GPU
vw::generate_mipmap(cmd, texture);
```

### Creating a Render Target

```cpp
// HDR color target
auto colorTarget = allocator->create_image_2D(
    vw::Width{1920}, vw::Height{1080},
    false,  // no mipmaps for render targets
    vk::Format::eR16G16B16A16Sfloat,
    vk::ImageUsageFlagBits::eColorAttachment |
    vk::ImageUsageFlagBits::eSampled);

// Depth buffer
auto depthTarget = allocator->create_image_2D(
    vw::Width{1920}, vw::Height{1080},
    false,
    vk::Format::eD32Sfloat,
    vk::ImageUsageFlagBits::eDepthStencilAttachment);
```

### Loading an Image from Disk

For manual control over the upload pipeline:

```cpp
// Load pixel data
auto desc = vw::load_image("textures/stone.png");

// Create GPU image
auto image = allocator->create_image_2D(
    desc.width, desc.height,
    true, vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);

// Upload via Transfer
vw::Transfer transfer;
transfer.copyBufferToImage(cmd, stagingBuffer.handle(), image, 0);

// Generate mipmaps
vw::generate_mipmap(cmd, image);
```

Or use the simpler `StagingBufferManager` path:

```cpp
vw::StagingBufferManager staging(device, allocator);
auto combined = staging.stage_image_from_path("textures/stone.png", true);
auto cmd = staging.fill_command_buffer();
// submit and wait...
```

### Saving a GPU Image to Disk

```cpp
vw::Transfer transfer;

// Track the image's current state
transfer.resourceTracker().track(vw::Barrier::ImageState{
    renderTarget, renderTarget->full_range(),
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite});

// Save handles transitions, copy, readback, and file writing
transfer.saveToFile(cmd, *allocator, queue, renderTarget, "screenshot.png");
```

## Common Pitfalls

### Missing usage flags at creation time

**Symptom:** Validation error when trying to use the image for an operation that was not declared in its usage flags.

**Cause:** Vulkan requires all intended uses to be declared when the image is created. You cannot add usage flags after creation.

**Fix:** Think through all the operations you will perform on the image and combine the necessary flags. The common combinations listed above cover most use cases.

### Forgetting `eTransferSrc` for mipmap generation

**Symptom:** Validation error during `generate_mipmap()`.

**Cause:** Mipmap generation blits from each level to the next. Without `eTransferSrc`, the image cannot be a blit source.

**Fix:** Include both transfer flags for mipmapped textures:

```cpp
vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc
```

### Not managing image layouts

**Symptom:** Garbled rendering, black textures, or validation errors about incorrect image layouts.

**Cause:** Vulkan images must be in specific layouts for different operations (e.g., `eShaderReadOnlyOptimal` for sampling, `eColorAttachmentOptimal` for rendering). Using an image in the wrong layout produces undefined results.

**Fix:** Use `ResourceTracker` to manage layout transitions:

```cpp
vw::Barrier::ResourceTracker tracker;
tracker.track(vw::Barrier::ImageState{
    image, image->full_range(),
    vk::ImageLayout::eUndefined,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::AccessFlagBits2::eNone});
tracker.request(vw::Barrier::ImageState{
    image, image->full_range(),
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderSampledRead});
tracker.flush(cmd);
```

### Using sRGB format for non-color data

**Symptom:** Normal maps or other data textures look wrong -- values are distorted.

**Cause:** sRGB formats (e.g., `eR8G8B8A8Srgb`) apply gamma correction when sampled. This is correct for color data that was authored in sRGB space, but corrupts linear data like normal maps or roughness maps.

**Fix:** Use `Unorm` format for linear data:

```cpp
// Color textures: use sRGB
vk::Format::eR8G8B8A8Srgb

// Normal maps, roughness, metallic: use linear
vk::Format::eR8G8B8A8Unorm
```

## Integration with Other Modules

| Next Step | How | See |
|-----------|-----|-----|
| Create views | `ImageViewBuilder(device, image)` | [Image Views](image-views.md) |
| Sample in shaders | Pair view with sampler via `CombinedImage` | [Samplers](samplers.md) |
| Allocate image memory | `Allocator::create_image_2D()` | [Allocator](../memory/allocator.md) |
| Upload data | `StagingBufferManager` or `Transfer` | [Staging & Transfers](../memory/staging.md) |
| Layout transitions | `ResourceTracker` | Synchronization module |
