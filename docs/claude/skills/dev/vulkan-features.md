# Vulkan 1.3 Features

## Device Setup

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

**Dynamic Rendering:** `cmd.beginRendering()` / `cmd.endRendering()`, no framebuffers

```cpp
cmd.beginRendering(vk::RenderingInfo{
    .renderArea = {{0, 0}, extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment
});
// draw
cmd.endRendering();
```

## Optional Features

**Descriptor Indexing:** Bindless resources, non-uniform indexing

**Ray Tracing:** BLAS/TLAS, RT shaders (requires buffer device address in allocator)

## Vulkan-Specific Anti-Pattern

Use Synchronization2 stage/access flags everywhere:
- `vk::PipelineStageFlagBits2::e...` (not v1 `vk::PipelineStageFlagBits::e...`)
- `vk::AccessFlagBits2::e...` (not v1 `vk::AccessFlagBits::e...`)

See CLAUDE.md for general anti-patterns (barriers, render pass, memory allocation).
