---
sidebar_position: 2
title: "Command Buffer"
---

# Command Buffer

## Overview

A **command buffer** is a recorded list of GPU instructions (draw calls,
dispatches, copies, barriers, ...).  In VulkanWrapper the native
`vk::CommandBuffer` is used directly for most operations, while
`CommandBufferRecorder` adds safety through RAII.

- **`CommandBufferRecorder`** -- an RAII guard that calls `begin()` on
  construction and `end()` on destruction, making it impossible to forget
  closing the recording scope.
- The raw `vk::CommandBuffer` handles returned by `CommandPool::allocate()` give
  you full access to every Vulkan command.

### Where it lives in the library

| Item | Header |
|------|--------|
| `CommandBufferRecorder` | `VulkanWrapper/Command/CommandBuffer.h` |
| `CommandPool::allocate` | `VulkanWrapper/Command/CommandPool.h` |

Both live in the `vw` namespace.

---

## API Reference

### Obtaining a command buffer

Command buffers are allocated from a `CommandPool`:

```cpp
auto pool = vw::CommandPoolBuilder(device).build();

// Allocate primary command buffers
std::vector<vk::CommandBuffer> cmds = pool.allocate(3);
```

The returned `vk::CommandBuffer` handles expose the entire vulkan.hpp command
recording API -- `beginRendering`, `draw`, `dispatch`, `copyBuffer`,
`pipelineBarrier2`, and so on.

### CommandBufferRecorder

| Method / Lifecycle | Description |
|--------------------|-------------|
| `CommandBufferRecorder(vk::CommandBuffer cmd)` | Calls `cmd.begin(...)`. The buffer must not already be in the recording state. |
| Destructor `~CommandBufferRecorder()` | Calls `cmd.end()`. |
| `traceRaysKHR(raygen, miss, hit, callable, width, height, depth)` | Convenience wrapper around `vkCmdTraceRaysKHR` for ray tracing dispatch. |

`CommandBufferRecorder` is non-copyable and non-movable. It is designed to be
used as a stack variable whose lifetime matches the recording scope.

### Recording manually (without the recorder)

You can also call `begin()` / `end()` yourself on the raw handle:

```cpp
auto cmd = pool.allocate(1)[0];

std::ignore = cmd.begin(
    vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

// ... record commands ...

std::ignore = cmd.end();
```

The `std::ignore =` pattern is used because VulkanWrapper is built with
`VULKAN_HPP_NO_EXCEPTIONS`, so `begin()` and `end()` return a
`vk::Result` that must be consumed.

---

## Usage Examples

### RAII recording with CommandBufferRecorder

```cpp
auto cmd = pool.allocate(1)[0];

{
    vw::CommandBufferRecorder recorder(cmd);

    cmd.beginRendering(renderingInfo);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->handle());
    cmd.draw(4, 1, 0, 0);
    cmd.endRendering();
}
// recorder destructor calls cmd.end() here

queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();
```

### One-time transfer commands

A very common pattern is to record a short-lived command buffer for a one-time
upload or copy:

```cpp
auto cmd = pool.allocate(1)[0];
std::ignore = cmd.begin(
    vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

vw::Transfer transfer;
transfer.copyBufferToImage(cmd, stagingBuffer.handle(), image, 0);

std::ignore = cmd.end();

queue.enqueue_command_buffer(cmd);
queue.submit({}, {}, {}).wait();
```

### Dynamic rendering (no VkRenderPass)

VulkanWrapper uses Vulkan 1.3 dynamic rendering exclusively.  There are no
`VkRenderPass` or `VkFramebuffer` objects.

```cpp
vk::RenderingAttachmentInfo colorAttachment =
    vk::RenderingAttachmentInfo()
        .setImageView(view->handle())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

vk::RenderingInfo renderingInfo =
    vk::RenderingInfo()
        .setRenderArea(vk::Rect2D({0, 0}, extent))
        .setLayerCount(1)
        .setColorAttachments(colorAttachment);

cmd.beginRendering(renderingInfo);
// bind pipeline, set viewport/scissor, draw ...
cmd.endRendering();
```

---

## Integration Patterns

### Command buffer lifecycle

```
allocate  -->  begin  -->  record commands  -->  end  -->  submit  -->  wait
```

After the fence signals, the buffer can be:
- **Re-recorded** (if the pool was created with `with_reset_command_buffer()`) by
  calling `cmd.reset()` followed by `begin()`.
- **Discarded** by simply not using it again; its memory returns to the pool
  when the pool is destroyed.

### Submission via Queue

VulkanWrapper's `Queue` class manages batching of command buffers:

```cpp
// Enqueue one or more command buffers
queue.enqueue_command_buffer(cmd);

// Submit returns a Fence you can wait on
auto fence = queue.submit(
    {},    // waitStages  (span<const vk::PipelineStageFlags>)
    {},    // waitSemaphores
    {});   // signalSemaphores

fence.wait();
```

### Pairing with ResourceTracker for barriers

Rather than manually calling `cmd.pipelineBarrier2()`, use the library's
`ResourceTracker` to build optimal barriers:

```cpp
vw::Barrier::ResourceTracker tracker;

tracker.track(vw::Barrier::ImageState{
    .image = image,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eUndefined,
    .stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone});

tracker.request(vw::Barrier::ImageState{
    .image = image,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite});

tracker.flush(cmd);  // emits a single vkCmdPipelineBarrier2
```

---

## Common Pitfalls

1. **Forgetting to call `end()` before submission.**
   If you record commands but forget `cmd.end()`, submission will fail with a
   validation error.  `CommandBufferRecorder` eliminates this class of bug.

2. **Recording into a buffer that is still in flight.**
   The GPU reads command buffers asynchronously after `submit()`.  You must wait
   for the submission fence before you reset or re-record the same buffer.

3. **Using `cmd.beginRenderPass()` instead of `cmd.beginRendering()`.**
   VulkanWrapper targets Vulkan 1.3 dynamic rendering.  The legacy
   `VkRenderPass` / `VkFramebuffer` path is not used and will trigger assertion
   failures in validation layers if mixed with dynamic rendering features.

4. **Using v1 synchronization (`vk::PipelineStageFlagBits`) instead of v2
   (`vk::PipelineStageFlagBits2`).**
   The library exclusively uses Synchronization2.  Mixing old and new types
   leads to subtle bugs.  Always use `PipelineStageFlagBits2` and
   `AccessFlags2`.

5. **Ignoring the `vk::Result` return value.**
   With `VULKAN_HPP_NO_EXCEPTIONS` enabled, `cmd.begin()` and `cmd.end()`
   return `vk::Result` instead of throwing.  Use `std::ignore =` to suppress
   the warning, or check the result explicitly in debug builds.
