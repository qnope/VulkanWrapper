# Vulkan Feature Negotiation

`DeviceFinder` (`Vulkan/DeviceFinder.h`) negotiates Vulkan features via chained `.with_*()` calls. See CLAUDE.md for anti-patterns.

## Device Setup

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()     // Required: Synchronization2 barriers
    .with_dynamic_rendering()     // Required: no VkRenderPass/VkFramebuffer
    .with_descriptor_indexing()   // Optional: bindless resources
    .with_ray_tracing()           // Optional: RT extensions + buffer device address
    .build();
```

## Required Features

| Feature | What It Enables | Used By |
|---------|-----------------|---------|
| `with_synchronization_2()` | `vk::PipelineStageFlagBits2`, `vk::AccessFlags2`, `cmd.pipelineBarrier2()` | `ResourceTracker` |
| `with_dynamic_rendering()` | `cmd.beginRendering()` / `cmd.endRendering()`, no framebuffers | All rendering code |

## Optional Features

| Feature | What It Enables | Used By |
|---------|-----------------|---------|
| `with_descriptor_indexing()` | Bindless resources, non-uniform indexing, variable descriptor counts | `BindlessMaterialManager`, `BindlessTextureManager` |
| `with_ray_tracing()` | RT pipeline, acceleration structures, buffer device address | `RayTracedScene`, `RayTracingPipeline`, `ShaderBindingTable` |

## Dynamic Rendering Example

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

## Queue Types

```cpp
.with_queue(vk::QueueFlagBits::eGraphics)   // Graphics + compute + transfer
.with_queue(vk::QueueFlagBits::eCompute)     // Async compute
.with_queue(vk::QueueFlagBits::eTransfer)    // Dedicated transfer (DMA)
```
