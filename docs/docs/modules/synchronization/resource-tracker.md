---
sidebar_position: 2
---

# Resource Tracker

The `ResourceTracker` (in namespace `vw::Barrier`) automatically manages GPU
resource state and emits optimal pipeline barriers. Instead of manually
calling `vkCmdPipelineBarrier2`, you **track** the current state of each
resource, **request** the state you need, and **flush** to generate the
minimal set of barriers.

## Why Use ResourceTracker?

In Vulkan, you must insert barriers between operations that read and write the
same resource. Getting barriers wrong causes data races, visual corruption, or
validation errors. ResourceTracker solves this by:

- Remembering the current state (layout, pipeline stage, access flags) of each
  image, buffer, and acceleration structure.
- Computing the minimal barrier when you request a new state.
- Batching multiple barriers into a single `vkCmdPipelineBarrier2` call.
- Tracking sub-resources independently (e.g., different mip levels can be in
  different layouts).

:::warning Anti-pattern
**Never** use raw `vkCmdPipelineBarrier` or `cmd.pipelineBarrier()`. Always use
`ResourceTracker` -- it is the only supported way to insert barriers in this
library.
:::

## Core API

```cpp
#include <VulkanWrapper/Synchronization/ResourceTracker.h>

namespace vw::Barrier {

class ResourceTracker {
  public:
    void track(const ResourceState &state);
    void request(const ResourceState &state);
    void flush(vk::CommandBuffer commandBuffer);
};

} // namespace vw::Barrier
```

The three methods form a simple pattern:

1. **`track(state)`** -- Tell the tracker what state a resource is currently
   in. Call this when you first create or transition a resource.
2. **`request(state)`** -- Tell the tracker what state you *need* the
   resource to be in next. The tracker computes the barrier internally.
3. **`flush(cmd)`** -- Record all pending barriers into the command buffer.
   After flushing, the tracked states are updated to match the requested
   states.

## Resource States

`ResourceState` is a `std::variant` of three concrete state types:

```cpp
using ResourceState =
    std::variant<ImageState, BufferState, AccelerationStructureState>;
```

### ImageState

```cpp
struct ImageState {
    vk::Image                image;
    vk::ImageSubresourceRange subresourceRange;
    vk::ImageLayout          layout;
    vk::PipelineStageFlags2  stage;
    vk::AccessFlags2         access;
};
```

The `subresourceRange` identifies which mip levels and array layers this state
applies to. Different sub-resources of the same image can be in different
states simultaneously.

### BufferState

```cpp
struct BufferState {
    vk::Buffer              buffer;
    vk::DeviceSize          offset;
    vk::DeviceSize          size;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2        access;
};
```

Like images, different byte ranges of the same buffer can be in different
states.

### AccelerationStructureState

```cpp
struct AccelerationStructureState {
    vk::AccelerationStructureKHR handle;
    vk::PipelineStageFlags2      stage;
    vk::AccessFlags2             access;
};
```

Used for ray tracing acceleration structures (BLAS / TLAS).

:::info Synchronization2 only
All stage and access fields use the **Synchronization2** types
(`vk::PipelineStageFlags2`, `vk::AccessFlags2`). Never use the legacy
`vk::PipelineStageFlagBits` or `vk::AccessFlagBits` -- they are Vulkan 1.0
types and are not supported.
:::

## The Track / Request / Flush Pattern

### Step-by-step Example

Suppose you have a freshly created image and want to use it as a color
attachment:

```cpp
vw::Barrier::ResourceTracker tracker;

// 1. Tell the tracker: "this image is currently in eUndefined layout"
tracker.track(vw::Barrier::ImageState{
    .image            = myImage->handle(),
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    .layout           = vk::ImageLayout::eUndefined,
    .stage            = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access           = vk::AccessFlagBits2::eNone
});

// 2. Tell the tracker: "I need it as a color attachment next"
tracker.request(vw::Barrier::ImageState{
    .image            = myImage->handle(),
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    .layout           = vk::ImageLayout::eColorAttachmentOptimal,
    .stage            = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access           = vk::AccessFlagBits2::eColorAttachmentWrite
});

// 3. Emit the barrier
tracker.flush(cmd);

// Now the image is ready for rendering.
// The tracker internally updated its state to eColorAttachmentOptimal.
```

### Multiple Resources in One Flush

You can track and request many resources before a single flush. The tracker
batches all transitions into one `vkCmdPipelineBarrier2` call:

```cpp
// Transition G-buffer images from attachment to shader-read
for (auto &gbufferImage : gbufferImages) {
    tracker.request(vw::Barrier::ImageState{
        .image            = gbufferImage->handle(),
        .subresourceRange = colorRange,
        .layout           = vk::ImageLayout::eShaderReadOnlyOptimal,
        .stage            = vk::PipelineStageFlagBits2::eFragmentShader,
        .access           = vk::AccessFlagBits2::eShaderSampledRead
    });
}

// One flush for all transitions
tracker.flush(cmd);
```

### State Persistence

After `flush()`, the tracker remembers the new state of every resource. You
do not need to call `track()` again after a flush -- just call `request()`
for the next transition:

```cpp
// Initial setup
tracker.track(imageState_undefined);
tracker.request(imageState_colorAttachment);
tracker.flush(cmd);

// Later in the same frame -- no need to re-track
tracker.request(imageState_shaderRead);
tracker.flush(cmd);
```

You only need `track()` when:
- A resource is first created.
- You know the resource's state has changed outside the tracker (e.g., by
  another queue or an external library).

## Common Transition Patterns

### Texture Upload

Upload pixel data from a staging buffer to a GPU image:

```cpp
// Image starts undefined
tracker.track(vw::Barrier::ImageState{
    .image = texture->handle(),
    .subresourceRange = colorRange,
    .layout = vk::ImageLayout::eUndefined,
    .stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// Transition to transfer destination
tracker.request(vw::Barrier::ImageState{
    .image = texture->handle(),
    .subresourceRange = colorRange,
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage  = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});
tracker.flush(cmd);

// Copy staging buffer -> image
cmd.copyBufferToImage(/* ... */);

// Transition to shader read
tracker.request(vw::Barrier::ImageState{
    .image = texture->handle(),
    .subresourceRange = colorRange,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage  = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
tracker.flush(cmd);
```

### Render Target to Shader Input

A common pattern in deferred rendering -- write to G-buffer attachments,
then read them in a lighting pass:

```cpp
// After geometry pass: images were written as color attachments.
// The tracker already knows they are in eColorAttachmentOptimal
// (from a previous track or request+flush).

// Request shader read for the lighting pass
tracker.request(vw::Barrier::ImageState{
    .image = positionBuffer->handle(),
    .subresourceRange = colorRange,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage  = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
tracker.flush(cmd);
```

### Buffer Barriers

Ensure a uniform buffer write is visible before the vertex shader reads it:

```cpp
tracker.track(vw::Barrier::BufferState{
    .buffer = uniformBuffer->handle(),
    .offset = 0,
    .size   = sizeof(UBO),
    .stage  = vk::PipelineStageFlagBits2::eHost,
    .access = vk::AccessFlagBits2::eHostWrite
});

tracker.request(vw::Barrier::BufferState{
    .buffer = uniformBuffer->handle(),
    .offset = 0,
    .size   = sizeof(UBO),
    .stage  = vk::PipelineStageFlagBits2::eVertexShader,
    .access = vk::AccessFlagBits2::eUniformRead
});
tracker.flush(cmd);
```

### Acceleration Structure Barriers

After building a TLAS, barrier before tracing rays:

```cpp
tracker.track(vw::Barrier::AccelerationStructureState{
    .handle = tlas.handle(),
    .stage  = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR
});

tracker.request(vw::Barrier::AccelerationStructureState{
    .handle = tlas.handle(),
    .stage  = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
tracker.flush(cmd);
```

## Sub-resource Tracking with IntervalSet

Internally, the tracker uses `ImageIntervalSet` and `BufferIntervalSet` to
manage fine-grained state. This means:

- **Different mip levels** of the same image can be in different layouts.
  For example, mip 0 can be `eColorAttachmentOptimal` while mips 1-N are
  `eTransferDstOptimal` during mipmap generation.
- **Different byte ranges** of the same buffer can have different access
  states.

When you track or request a sub-resource range, the tracker automatically
splits, merges, and updates intervals as needed. You do not interact with
`IntervalSet` directly -- it is an implementation detail.

### Example: Per-Mip Transitions

```cpp
// Generate mipmaps: blit from mip N to mip N+1
for (uint32_t mip = 1; mip < mipLevels; ++mip) {
    // Source mip: transition to transfer source
    tracker.request(vw::Barrier::ImageState{
        .image = texture->handle(),
        .subresourceRange = {vk::ImageAspectFlagBits::eColor,
                             mip - 1, 1, 0, 1},
        .layout = vk::ImageLayout::eTransferSrcOptimal,
        .stage  = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferRead
    });

    // Destination mip: transition to transfer destination
    tracker.request(vw::Barrier::ImageState{
        .image = texture->handle(),
        .subresourceRange = {vk::ImageAspectFlagBits::eColor,
                             mip, 1, 0, 1},
        .layout = vk::ImageLayout::eTransferDstOptimal,
        .stage  = vk::PipelineStageFlagBits2::eTransfer,
        .access = vk::AccessFlagBits2::eTransferWrite
    });

    tracker.flush(cmd);

    // Blit from mip-1 to mip
    cmd.blitImage(/* ... */);
}
```

## Integration with Transfer

The `Transfer` class (`VulkanWrapper/Memory/Transfer.h`) embeds its own
`ResourceTracker` and manages barriers automatically for copy and blit
operations:

```cpp
vw::Transfer transfer;

// Blit with automatic barrier management
transfer.blit(cmd, srcImage, dstImage);

// Access the embedded tracker for additional manual transitions
auto &tracker = transfer.resourceTracker();
tracker.request(/* ... */);
tracker.flush(cmd);
```

When using `Transfer`, you generally do not need a separate `ResourceTracker`
for the resources it manages.

## Best Practices

1. **Call `track()` once** when a resource is first created or when its state
   is unknown. After that, let `request()` + `flush()` maintain the state.
2. **Batch transitions** -- request all needed transitions before a single
   `flush()`. This produces fewer, wider barriers.
3. **Use precise sub-resource ranges** -- tracking the exact mip level or
   buffer range avoids unnecessary full-resource barriers.
4. **Always use Synchronization2 types** -- `vk::PipelineStageFlagBits2` and
   `vk::AccessFlags2`, never the legacy v1 enums.
5. **Let Transfer handle copy barriers** -- prefer `Transfer::blit()`,
   `Transfer::copyBufferToImage()`, etc. over manual track/request for
   copy operations.
