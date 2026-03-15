---
sidebar_position: 2
title: "Image Views"
---

# Image Views

An `ImageView` defines how a shader or rendering operation interprets an `Image`. While an `Image` is a raw block of GPU memory with format and dimensions, an `ImageView` specifies which subset of that image to use (mip levels, array layers), how to interpret it (2D, cube, array), and which aspect to read (color, depth, stencil).

## Design Rationale

Vulkan requires an `VkImageView` for almost every image operation: binding a texture to a descriptor set, using an image as a render target, or reading from an input attachment. The view decouples *how an image is stored* from *how it is accessed*, allowing a single image to be used in multiple ways:

- A cubemap image (6 layers) can be viewed as a `eCube` for environment mapping, or as six individual `e2D` views for rendering into each face.
- A mipmapped image can be viewed at all mip levels for sampling, or at a single level for writing.
- A depth-stencil image can be viewed with the depth aspect only (for shadow sampling) or the stencil aspect only.

`ImageViewBuilder` follows the library's builder pattern, defaulting to the most common case (2D view of the entire image with color aspect) while allowing customization through method chaining.

## API Reference

### ImageViewBuilder

```cpp
namespace vw {

class ImageViewBuilder {
public:
    ImageViewBuilder(std::shared_ptr<const Device> device,
                     std::shared_ptr<const Image> image);

    ImageViewBuilder &setImageType(vk::ImageViewType imageViewType);

    std::shared_ptr<const ImageView> build();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Image/ImageView.h`

#### Constructor

Takes a `Device` and `Image`, both by `shared_ptr`. The resulting `ImageView` keeps the `Image` alive through shared ownership.

#### `setImageType(vk::ImageViewType imageViewType)`

Sets the view type. Default: `vk::ImageViewType::e2D`.

Available types:

| Type | Description | Typical Use |
|------|-------------|-------------|
| `e1D` | 1D texture view | Lookup tables |
| `e2D` | 2D texture view (default) | Standard textures, render targets |
| `e3D` | 3D texture/volume view | Volume rendering |
| `eCube` | Cubemap (6 layers as faces) | Environment maps, skyboxes |
| `e1DArray` | Array of 1D textures | Multiple lookup tables |
| `e2DArray` | Array of 2D textures | Texture arrays, shadow cascades |
| `eCubeArray` | Array of cubemaps | Irradiance probe arrays |

#### `build()`

Creates the `ImageView` and returns it as `shared_ptr<const ImageView>`. The view's subresource range covers all mip levels and array layers of the source image by default.

The component mapping is identity (R maps to R, G to G, B to B, A to A).

### ImageView

```cpp
namespace vw {

class ImageView : public ObjectWithUniqueHandle<vk::UniqueImageView> {
public:
    std::shared_ptr<const Image> image() const noexcept;
    vk::ImageSubresourceRange subresource_range() const noexcept;
};

} // namespace vw
```

#### `handle()`

Returns the raw `vk::ImageView` handle (inherited from `ObjectWithUniqueHandle`). This is what you pass to Vulkan structures like `vk::RenderingAttachmentInfo` and `vk::DescriptorImageInfo`.

#### `image()`

Returns a `shared_ptr` to the source `Image`. The view keeps the image alive -- you do not need to hold a separate reference to the image.

#### `subresource_range()`

Returns the `vk::ImageSubresourceRange` this view covers:

```cpp
auto range = view->subresource_range();
// range.aspectMask     - which aspect (color, depth, stencil)
// range.baseMipLevel   - first mip level included
// range.levelCount     - number of mip levels included
// range.baseArrayLayer - first array layer included
// range.layerCount     - number of array layers included
```

## Usage Patterns

### Basic 2D View

The most common case -- creating a view for a regular 2D texture or render target:

```cpp
auto view = vw::ImageViewBuilder(device, image).build();
```

This creates a 2D view covering all mip levels and array layers, with the color aspect.

### Cubemap View

For an image created with 6 array layers:

```cpp
auto cubeView = vw::ImageViewBuilder(device, cubemapImage)
    .setImageType(vk::ImageViewType::eCube)
    .build();
```

### Using as a Render Target

Image views are used as color and depth attachments with dynamic rendering:

```cpp
auto colorView = vw::ImageViewBuilder(device, colorImage).build();

vk::RenderingAttachmentInfo colorAttachment{
    .imageView = colorView->handle(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}
};

vk::RenderingInfo renderInfo{
    .renderArea = {.extent = colorImage->extent2D()},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment
};

cmd.beginRendering(renderInfo);
// draw...
cmd.endRendering();
```

### Using as a Sampled Texture

Image views are paired with samplers to create descriptor bindings:

```cpp
auto textureView = vw::ImageViewBuilder(device, texture).build();

vk::DescriptorImageInfo imageInfo{
    .sampler = sampler->handle(),
    .imageView = textureView->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};
```

Or use `CombinedImage` for a cleaner interface -- see [Samplers](samplers.md).

### Using as a Depth Attachment

```cpp
auto depthView = vw::ImageViewBuilder(device, depthImage).build();

vk::RenderingAttachmentInfo depthAttachment{
    .imageView = depthView->handle(),
    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = vk::ClearDepthStencilValue{1.0f, 0}
};

vk::RenderingInfo renderInfo{
    .renderArea = {.extent = depthImage->extent2D()},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .pDepthAttachment = &depthAttachment
};
```

### Swapchain Image Views

The `Swapchain` pre-creates image views for all its images. You do not need to create them manually:

```cpp
auto views = swapchain.image_views();
auto view = views[imageIndex];  // shared_ptr<const ImageView>
```

### `to_handle` Range Adaptor

When you have a collection of image views and need the raw handles (for example, for an array of descriptor bindings), use the `vw::to_handle` range adaptor:

```cpp
auto handles = views | vw::to_handle | std::ranges::to<std::vector>();
// handles is std::vector<vk::ImageView>
```

## Subresource Ranges Explained

A subresource range identifies a rectangular region within the image's mip/layer space:

```
                  Layer 0    Layer 1    Layer 2    ...
Mip Level 0    [  1024x1024  ] [  1024x1024  ] [  1024x1024  ]
Mip Level 1    [  512x512    ] [  512x512    ] [  512x512    ]
Mip Level 2    [  256x256    ] [  256x256    ] [  256x256    ]
...
```

A view selects a rectangular slice of this grid. For a basic 2D texture, the default covers everything: all mip levels, one layer, color aspect. For more specific uses, the `Image` class provides helper methods to construct ranges:

```cpp
image->full_range();                    // All levels, all layers
image->mip_level_range(MipLevel{2});    // Only mip level 2
```

### Aspect Flags

The aspect determines which component of a combined format is accessed:

| Aspect | Use |
|--------|-----|
| `eColor` | Color images (default for most formats) |
| `eDepth` | Depth component of depth or depth-stencil images |
| `eStencil` | Stencil component of depth-stencil images |

For a depth-stencil image (e.g., `eD24UnormS8Uint`), you need separate views for the depth and stencil aspects if you want to sample them independently in shaders. The `ImageViewBuilder` automatically infers the correct aspect from the image's format for the common cases.

## Common Pitfalls

### Creating a cubemap view without 6 layers

**Symptom:** Validation error about layer count mismatch.

**Cause:** A `eCube` view requires exactly 6 array layers in the source image. If the image has fewer layers, the view creation fails.

**Fix:** Create the source image with 6 array layers. VulkanWrapper's `create_image_2D` creates single-layer images; for cubemaps you may need to use the lower-level Allocator or a custom image creation path.

### Mismatching view type and image type

**Symptom:** Validation error about incompatible view type.

**Cause:** For example, trying to create a `e3D` view from a 2D image, or a `e2D` view from a 3D image.

**Fix:** The view type must be compatible with the image's type. A 2D image can have `e2D` or `e2DArray` views. A 3D image can have `e3D` views.

### Forgetting that views keep images alive

**Symptom:** Not a bug per se, but unexpected memory usage.

**Cause:** `ImageView` holds a `shared_ptr<const Image>`, keeping the GPU memory allocated as long as any view exists.

**Fix:** This is usually the correct behavior. If you need to free GPU memory, make sure all views are destroyed first.

### Using the wrong layout with a view

**Symptom:** Black textures, corrupted rendering, validation errors about image layout.

**Cause:** The `vk::ImageLayout` you specify in `vk::DescriptorImageInfo` or `vk::RenderingAttachmentInfo` must match the layout the image was transitioned to.

**Fix:** Use `ResourceTracker` to ensure the image is in the correct layout before use:

- `eShaderReadOnlyOptimal` for sampled textures
- `eColorAttachmentOptimal` for color render targets
- `eDepthAttachmentOptimal` for depth buffers

## Integration with Other Modules

| What | How | See |
|------|-----|-----|
| Create images | `Allocator::create_image_2D()` | [Images](images.md), [Allocator](../memory/allocator.md) |
| Pair with sampler | `CombinedImage(imageView, sampler)` | [Samplers](samplers.md) |
| Use as render target | Pass to `vk::RenderingAttachmentInfo` | Dynamic rendering |
| Layout transitions | `ResourceTracker` | Synchronization module |
| Swapchain views | `swapchain.image_views()` | [Swapchain & Window](../vulkan/swapchain.md) |
