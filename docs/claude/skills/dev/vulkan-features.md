# Vulkan 1.3 Features

## Device Setup

`DeviceFinder` (`Vulkan/DeviceFinder.h`) negotiates features via chained `.with_*()` calls:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()     // Required: modern barriers
    .with_dynamic_rendering()     // Required: no VkRenderPass
    .with_descriptor_indexing()   // Optional: bindless
    .with_ray_tracing()           // Optional: RT extensions
    .build();
```

## Required Features

**Synchronization2:** 64-bit pipeline stages/access flags, `cmd.pipelineBarrier2()`
- Always use `vk::PipelineStageFlagBits2` (not v1 `vk::PipelineStageFlagBits`)
- Always use `vk::AccessFlags2` (not v1 `vk::AccessFlagBits`)
- Used by `ResourceTracker` internally

**Dynamic Rendering:** `cmd.beginRendering()` / `cmd.endRendering()`, no framebuffers

```cpp
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .clearValue = vk::ClearValue{vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}}
};

cmd.beginRendering(vk::RenderingInfo{
    .renderArea = {{0, 0}, extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment
});
// draw commands
cmd.endRendering();
```

## Optional Features

**Descriptor Indexing:** Bindless resources, non-uniform indexing
- Used by `BindlessMaterialManager` and `BindlessTextureManager`
- Enable via `.with_descriptor_indexing()` on `DeviceFinder`

**Ray Tracing:** BLAS/TLAS, RT shaders
- Requires buffer device address (always enabled in `Allocator`)
- Enable via `.with_ray_tracing()` on `DeviceFinder`
- See [ray-tracing.md](ray-tracing.md) for details

## Anti-Patterns (DO NOT)

- **DO NOT** use v1 stage/access flags: `vk::PipelineStageFlagBits::e...` -> use `vk::PipelineStageFlagBits2::e...`
- **DO NOT** use `cmd.beginRenderPass()` -> use `cmd.beginRendering()`
- **DO NOT** create `VkFramebuffer` or `VkRenderPass` objects -- dynamic rendering eliminates these
- **DO NOT** use `Vk` C prefix types -> use `vk::` C++ bindings

See CLAUDE.md "Anti-Patterns" section for the full list.
