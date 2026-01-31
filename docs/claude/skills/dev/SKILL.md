# Agent Developer Skills Guide

Quick reference for AI agents working on this VulkanWrapper codebase.

## Core Principles

1. **Modern C++23** - concepts over SFINAE, ranges over loops
2. **Vulkan 1.3** - dynamic rendering, synchronization2, no render passes
3. **Type safety** - strong types, compile-time validation
4. **RAII everywhere** - no manual cleanup

---

## Reference Documentation

| Topic                 | File                                         | Key Points                                                  |
|-----------------------|----------------------------------------------|-------------------------------------------------------------|
| **Modern C++**        | [modern-cpp.md](modern-cpp.md)               | Concepts, ranges, constexpr/consteval, std::variant         |
| **Memory Allocation** | [memory-allocation.md](memory-allocation.md) | Allocator, Buffer types, BufferList, random sampling        |
| **Barriers**          | [barriers.md](barriers.md)                   | ResourceTracker, layout transitions, Transfer API           |
| **Vulkan 1.3**        | [vulkan-features.md](vulkan-features.md)     | Dynamic rendering, Synchronization2, feature selection      |
| **Ray Tracing**       | [ray-tracing.md](ray-tracing.md)             | BLAS/TLAS, RayTracedScene, SBT, RT pipelines                |
| **Shaders**           | [shaders.md](shaders.md)                     | ShaderCompiler, includes, push constants, descriptors       |
| **Patterns**          | [patterns.md](patterns.md)                   | ScreenSpacePass, builders, error handling, troubleshooting  |

---

## Quick Examples

### Concept (not SFINAE)

```cpp
template <typename T>
concept Vertex = std::is_standard_layout_v<T> && requires(T x) {
    { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
};

void process(std::span<const T> data) requires(HostVisible) { ... }
```

### Ranges (not loops)

```cpp
auto vertices = std::views::zip(positions, normals, uvs)
    | construct<Vertex3D>
    | to<std::vector>;

std::ranges::find_if(devices, hasFeature);
std::erase_if(items, predicate);
```

### Memory Allocation

```cpp
auto allocator = AllocatorBuilder(instance, device).build();
auto buffer = allocate_vertex_buffer<Vertex3D, false>(*allocator, count);
auto image = allocator->create_image_2D(Width{1920}, Height{1080}, false, format, usage);
```

### ResourceTracker

```cpp
vw::Barrier::ResourceTracker tracker;
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.flush(cmd);
```

### Dynamic Rendering

```cpp
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = view, .imageLayout = eColorAttachmentOptimal,
    .loadOp = eClear, .storeOp = eStore, .clearValue = clearColor
};
cmd.beginRendering(vk::RenderingInfo{.renderArea = area, .layerCount = 1,
    .colorAttachmentCount = 1, .pColorAttachments = &colorAttachment});
// draw...
cmd.endRendering();
```

---

## Anti-Patterns Summary

| DON'T | DO |
|-------|-----|
| `std::enable_if_t<...>` | `requires(Concept)` |
| `for (i = 0; i < n; ++i)` | `data \| std::views::transform(fn)` |
| `cmd.pipelineBarrier(...)` | `tracker.request(...); tracker.flush(cmd);` |
| `vkCmdBeginRenderPass(...)` | `cmd.beginRendering(...)` |
| `vkCreateBuffer + vkAllocateMemory` | `allocator->allocate_buffer(...)` |
| `vk::PipelineStageFlagBits::e...` | `vk::PipelineStageFlagBits2::e...` |

---

## End-to-End Integration Example

Complete workflow from initialization to rendering:

```cpp
// 1. Instance with validation
auto instance = InstanceBuilder()
    .setDebug()
    .setApiVersion(ApiVersion::e13)
    .build();

// 2. Device with required features
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_presentation(surface)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// 3. Memory allocator
auto allocator = AllocatorBuilder(instance, device).build();

// 4. Command pool
auto commandPool = CommandPoolBuilder(device)
    .set_queue_family(device->graphics_queue().familyIndex())
    .set_flags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    .build();
auto cmdBuffers = commandPool.allocate(frameCount);

// 5. Create resources
auto vertexBuffer = allocate_vertex_buffer<Vertex3D, false>(*allocator, vertexCount);
auto image = allocator->create_image_2D(
    Width{1920}, Height{1080}, false,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
auto imageView = ImageViewBuilder(device, image).build();

// 6. Render loop
{
    CommandBufferRecorder recorder(cmdBuffers[frameIndex]);
    vw::Barrier::ResourceTracker tracker;

    // Track initial state
    tracker.track(ImageState{
        image->handle(), image->full_range(),
        vk::ImageLayout::eUndefined,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone
    });

    // Request rendering state
    tracker.request(ImageState{
        image->handle(), image->full_range(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite
    });
    tracker.flush(recorder.handle());

    // Render with dynamic rendering
    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = imageView->handle(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore
    };
    recorder.handle().beginRendering({
        .renderArea = {{0, 0}, extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    });

    recorder.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    recorder.handle().bindVertexBuffers(0, vertexBuffer.handle(), {0});
    recorder.handle().draw(vertexCount, 1, 0, 0);
    recorder.handle().endRendering();

    // Transition to presentable
    tracker.track(ImageState{...eColorAttachmentOptimal...});
    tracker.request(ImageState{...ePresentSrcKHR...});
    tracker.flush(recorder.handle());
}

// 7. Submit and present
device->graphics_queue().submit(submitInfo);
swapchain->present(device->graphics_queue(), imageIndex);
```

---

## Key Source Files

| Purpose | Path |
|---------|------|
| Allocator | `VulkanWrapper/include/VulkanWrapper/Memory/Allocator.h` |
| Buffer templates | `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h` |
| ResourceTracker | `VulkanWrapper/include/VulkanWrapper/Synchronization/ResourceTracker.h` |
| Transfer API | `VulkanWrapper/include/VulkanWrapper/Memory/Transfer.h` |
| Range adaptors | `VulkanWrapper/include/VulkanWrapper/Utils/Algos.h` |
| Vertex concept | `VulkanWrapper/include/VulkanWrapper/Descriptors/Vertex.h` |
| ScreenSpacePass | `VulkanWrapper/include/VulkanWrapper/RenderPass/ScreenSpacePass.h` |
| Error handling | `VulkanWrapper/include/VulkanWrapper/Utils/Error.h` |
| ShaderCompiler | `VulkanWrapper/include/VulkanWrapper/Shader/ShaderCompiler.h` |
| RayTracedScene | `VulkanWrapper/include/VulkanWrapper/RayTracing/RayTracedScene.h` |
