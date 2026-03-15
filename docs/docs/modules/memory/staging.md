---
sidebar_position: 3
title: "Staging & Transfers"
---

# Staging & Transfers

GPU-optimal memory (device-local) is not directly accessible from the CPU. To get data onto the GPU, you write it to a host-visible staging buffer, then issue a GPU copy command. VulkanWrapper provides `StagingBufferManager` for batching multiple uploads into a single command buffer, and `Transfer` for managing copy operations with automatic barrier management.

## Design Rationale

Uploading data to the GPU is one of the most common operations in a Vulkan application -- vertices, indices, textures, and uniform data all need to make this journey. The raw Vulkan approach requires:

1. Creating a staging buffer with host-visible memory.
2. Mapping the staging buffer and writing data.
3. Recording a `vkCmdCopyBuffer` or `vkCmdCopyBufferToImage` command.
4. Inserting memory barriers so the GPU reads the data only after the copy finishes.
5. Submitting the command buffer and waiting.
6. Destroying the staging buffer.

`StagingBufferManager` automates steps 1-3: it pools staging memory across multiple uploads, records all copy commands into a single command buffer, and uses compile-time checks to verify that destination buffers support transfer operations.

`Transfer` automates step 4: it embeds a `ResourceTracker` that generates the correct pipeline barriers for each copy operation, so you never need to manually specify source/destination stages and access masks.

## API Reference

### StagingBufferManager

```cpp
namespace vw {

class StagingBufferManager {
public:
    StagingBufferManager(std::shared_ptr<const Device> device,
                         std::shared_ptr<const Allocator> allocator);

    template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
    void fill_buffer(std::span<const T> data,
                     const Buffer<T, HostVisible, Usage> &buffer,
                     uint32_t offset_dst_buffer);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    [[nodiscard]] CombinedImage
    stage_image_from_path(const std::filesystem::path &path, bool mipmaps);
};

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/StagingBufferManager.h`

#### Constructor

Takes a `Device` and `Allocator`. The manager creates its own internal command pool and staging buffer pool.

#### `fill_buffer(data, buffer, offset_dst_buffer)`

Queues a copy of `data` into `buffer` at the given element offset. This does not execute immediately -- the copy command is recorded when you call `fill_command_buffer()`.

The template performs a compile-time `static_assert` to verify that the destination buffer's usage flags include `VK_BUFFER_USAGE_TRANSFER_DST_BIT`. If they do not, you get a clear compilation error rather than a runtime validation error.

```cpp
vw::StagingBufferManager staging(device, allocator);

std::vector<Vertex> vertices = /* ... */;
staging.fill_buffer<Vertex>(vertices, vertexBuffer, 0);

std::vector<uint32_t> indices = /* ... */;
staging.fill_buffer<uint32_t>(indices, indexBuffer, 0);
```

#### `fill_command_buffer()`

Records all queued copy operations into a command buffer and returns it. The returned command buffer is in the executable state -- you must submit it to a queue and wait for completion before using the uploaded data.

```cpp
auto cmd = staging.fill_command_buffer();

auto &queue = device->graphicsQueue();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();
```

#### `stage_image_from_path(path, mipmaps)`

Loads an image from disk, creates a GPU image and image view, stages the pixel data, and optionally prepares for mipmap generation. Returns a `CombinedImage` (image view + sampler) ready for use after the command buffer is submitted.

```cpp
auto combined = staging.stage_image_from_path("texture.png", true);
// After fill_command_buffer() is submitted and completes,
// combined can be used in descriptor sets
```

### Transfer

```cpp
namespace vw {

class Transfer {
public:
    Transfer() = default;

    void blit(vk::CommandBuffer cmd,
              const std::shared_ptr<const Image> &src,
              const std::shared_ptr<const Image> &dst,
              std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt,
              std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt,
              vk::Filter filter = vk::Filter::eLinear);

    void copyBuffer(vk::CommandBuffer cmd,
                    vk::Buffer src, vk::Buffer dst,
                    vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
                    vk::DeviceSize size);

    void copyBufferToImage(
        vk::CommandBuffer cmd, vk::Buffer src,
        const std::shared_ptr<const Image> &dst, vk::DeviceSize srcOffset,
        std::optional<vk::ImageSubresourceRange> dstSubresource = std::nullopt);

    void copyImageToBuffer(
        vk::CommandBuffer cmd, const std::shared_ptr<const Image> &src,
        vk::Buffer dst, vk::DeviceSize dstOffset,
        std::optional<vk::ImageSubresourceRange> srcSubresource = std::nullopt);

    void saveToFile(vk::CommandBuffer cmd, const Allocator &allocator,
                    Queue &queue, const std::shared_ptr<const Image> &image,
                    const std::filesystem::path &path,
                    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

    [[nodiscard]] Barrier::ResourceTracker &resourceTracker();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/Transfer.h`

`Transfer` records copy operations into a command buffer **and** manages the required image/buffer layout transitions through its embedded `ResourceTracker`. Each method automatically inserts the correct barriers so the source is readable and the destination is writable at the time of the copy.

#### `blit()`

Blits (scaled copy with format conversion) from source image to destination image. Both images must be tracked by the embedded `ResourceTracker`. The `filter` parameter controls the scaling algorithm (default: `eLinear`).

Optional subresource ranges let you blit specific mip levels or array layers. When omitted, the full image range is used.

```cpp
vw::Transfer transfer;
transfer.blit(cmd, sourceImage, destinationImage);
```

#### `copyBuffer()`

Copies a region of bytes from one buffer to another:

```cpp
transfer.copyBuffer(cmd,
    srcBuffer.handle(), dstBuffer.handle(),
    0,    // srcOffset
    0,    // dstOffset
    size);
```

#### `copyBufferToImage()`

Copies pixel data from a buffer into an image. The image is automatically transitioned to the correct layout for receiving the copy:

```cpp
transfer.copyBufferToImage(cmd,
    stagingBuffer.handle(),
    destinationImage,
    0);  // srcOffset in buffer
```

#### `copyImageToBuffer()`

Copies pixel data from an image into a buffer. The image is transitioned to `eTransferSrcOptimal` before the copy:

```cpp
transfer.copyImageToBuffer(cmd,
    sourceImage,
    readbackBuffer.handle(),
    0);  // dstOffset in buffer
```

#### `saveToFile()`

A high-level utility that saves a GPU image to disk. Handles the entire process:

1. Creates a host-visible staging buffer large enough for the image.
2. Transitions the image to `eTransferSrcOptimal`.
3. Copies the image to the staging buffer.
4. Transitions the image to `finalLayout` (default: `ePresentSrcKHR`).
5. Ends the command buffer, submits it, and waits for completion.
6. Reads back the pixels and converts color format if needed (BGRA to RGBA).
7. Writes the file using `ImageLoader` (format determined by file extension: `.png`, `.bmp`, `.jpg`).

**Important:** The `ResourceTracker` must already have the image tracked with its current state before calling `saveToFile()`. The command buffer must be in the recording state.

```cpp
vw::Transfer transfer;

// Track the image's current state
transfer.resourceTracker().track(vw::Barrier::ImageState{
    image, image->full_range(),
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite});

transfer.saveToFile(cmd, *allocator, queue, image, "screenshot.png");
```

#### `resourceTracker()`

Returns a reference to the embedded `Barrier::ResourceTracker`. Use this to manually track additional resources or set up initial image states before performing transfers.

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

Low-level image file I/O using SDL_image:

- `load_image()` loads an image file and returns raw pixel data with dimensions.
- `save_image()` writes RGBA8 pixel data to a file (format determined by extension).

These are used internally by `StagingBufferManager::stage_image_from_path()` and `Transfer::saveToFile()`. You can also use them directly for custom upload/download pipelines.

### Mipmap Generation

```cpp
namespace vw {

void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image);

} // namespace vw
```

**Header:** `VulkanWrapper/Image/Mipmap.h`

Generates a full mip chain from an image's base level using successive blit operations. The image must have been created with `mipmap = true` (which allocates space for the full chain) and must include both `eTransferSrc` and `eTransferDst` in its usage flags.

```cpp
// After uploading base mip level...
vw::generate_mipmap(cmd, textureImage);
```

## Usage Patterns

### Batched Buffer Upload

The most common pattern -- uploading vertex and index data for a mesh:

```cpp
vw::StagingBufferManager staging(device, allocator);

// Queue multiple uploads
staging.fill_buffer<Vertex>(vertices, vertexBuffer, 0);
staging.fill_buffer<uint32_t>(indices, indexBuffer, 0);

// Record and submit
auto cmd = staging.fill_command_buffer();
auto &queue = device->graphicsQueue();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();

// vertexBuffer and indexBuffer are now ready for rendering
```

### Texture Upload with Mipmaps

```cpp
vw::StagingBufferManager staging(device, allocator);

// Load texture and create GPU image with mipmaps
auto combined = staging.stage_image_from_path("diffuse.png", true);

// Submit all uploads
auto cmd = staging.fill_command_buffer();
auto &queue = device->graphicsQueue();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();

// combined is now ready for descriptor binding
```

### Saving a Rendered Image to Disk

For screenshots or offline rendering results:

```cpp
vw::Transfer transfer;

// Track the rendered image's current state
transfer.resourceTracker().track(vw::Barrier::ImageState{
    renderTarget, renderTarget->full_range(),
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite});

// Save (handles all transitions, copy, readback, and file writing)
transfer.saveToFile(cmd, *allocator, queue, renderTarget, "output.png");
```

### Image-to-Image Blit

For downscaling, format conversion, or copying between images:

```cpp
vw::Transfer transfer;

// Track both images
auto &tracker = transfer.resourceTracker();
tracker.track(vw::Barrier::ImageState{
    srcImage, srcImage->full_range(),
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderSampledRead});

tracker.track(vw::Barrier::ImageState{
    dstImage, dstImage->full_range(),
    vk::ImageLayout::eUndefined,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::AccessFlagBits2::eNone});

// Blit with automatic barrier management
transfer.blit(cmd, srcImage, dstImage);
```

### Manual Staging (Fine-Grained Control)

When `StagingBufferManager` does not fit your needs:

```cpp
// 1. Create staging buffer
auto staging = vw::create_buffer<Vertex, true, vw::StagingBufferUsage>(
    *allocator, vertices.size());

// 2. Write data to staging
staging.write(vertices, 0);

// 3. Record copy command
vk::BufferCopy region{
    .srcOffset = 0,
    .dstOffset = 0,
    .size = staging.size_bytes()
};
cmd.copyBuffer(staging.handle(), gpuBuffer.handle(), region);

// 4. Use ResourceTracker for the barrier
vw::Barrier::ResourceTracker tracker;
tracker.track(vw::Barrier::BufferState{
    gpuBuffer.handle(), gpuBuffer.size_bytes(),
    vk::PipelineStageFlagBits2::eTransfer,
    vk::AccessFlagBits2::eTransferWrite});
tracker.request(vw::Barrier::BufferState{
    gpuBuffer.handle(), gpuBuffer.size_bytes(),
    vk::PipelineStageFlagBits2::eVertexInput,
    vk::AccessFlagBits2::eVertexAttributeRead});
tracker.flush(cmd);
```

## Common Pitfalls

### Using uploaded data before the command buffer completes

**Symptom:** Rendering shows garbage data, zeros, or the previous frame's content.

**Cause:** `fill_buffer()` only queues the copy -- the data is not on the GPU until the command buffer is submitted and the fence is waited on.

**Fix:** Always wait for the fence before using uploaded data:

```cpp
auto cmd = staging.fill_command_buffer();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();  // Data is now on the GPU
```

### Forgetting to track image state before `saveToFile()`

**Symptom:** Validation errors about invalid image layout transitions, or corrupted output files.

**Cause:** `saveToFile()` uses the `ResourceTracker` to transition the image. If the image's current state is not tracked, the tracker generates incorrect barriers.

**Fix:** Track the image before calling `saveToFile()`:

```cpp
transfer.resourceTracker().track(vw::Barrier::ImageState{
    image, image->full_range(),
    currentLayout, currentStage, currentAccess});
```

### Missing `eTransferDst` on destination buffers

**Symptom:** Compile error from `static_assert` in `fill_buffer()`.

**Cause:** The destination buffer's usage flags do not include `eTransferDst`, so it cannot receive copy operations.

**Fix:** All VulkanWrapper usage constants (`VertexBufferUsage`, `IndexBufferUsage`, etc.) already include `eTransferDst`. If you are using custom flags, make sure to include it.

### Destroying the staging buffer before the copy completes

**Symptom:** Corrupted data on the GPU, or a crash.

**Cause:** If you create a staging buffer in a local scope and it goes out of scope before the GPU finishes the copy, the source data is freed while the GPU is still reading it.

**Fix:** Keep staging buffers alive until the fence is waited on. `StagingBufferManager` handles this automatically -- its internal staging buffers live as long as the manager.

### Forgetting `eTransferSrc` for mipmap generation

**Symptom:** Validation error during `generate_mipmap()`.

**Cause:** Mipmap generation blits from one level to the next, requiring the image to be a blit source.

**Fix:** Include both `eTransferSrc` and `eTransferDst` in the image's usage flags when creating it with `mipmap = true`:

```cpp
auto image = allocator->create_image_2D(
    width, height, true, format,
    vk::ImageUsageFlagBits::eSampled |
    vk::ImageUsageFlagBits::eTransferDst |
    vk::ImageUsageFlagBits::eTransferSrc);
```

## Integration with Other Modules

| Dependency | Role | See |
|-----------|------|-----|
| `Allocator` | Creates staging buffers and images | [Allocator](allocator.md) |
| `Buffer<>` | Source and destination for copies | [Buffers](buffers.md) |
| `Image` | Source/destination for image transfers | [Images](../image/images.md) |
| `ResourceTracker` | Automatic barrier generation | Synchronization module |
| `Queue` / `Fence` | Command submission and synchronization | [Device & GPU Selection](../vulkan/device.md) |
