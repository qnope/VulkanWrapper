---
sidebar_position: 2
---

# Command Buffer

Command buffers record GPU commands for later submission.

## Overview

```cpp
#include <VulkanWrapper/Command/CommandBuffer.h>

auto cmd = pool->allocate();

// Record commands
{
    CommandBufferRecorder recorder(cmd);
    // Commands recorded here
    recorder.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    recorder.draw(3, 1, 0, 0);
}  // Recording ends automatically
```

## CommandBuffer Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::CommandBuffer` | Get raw handle |
| `reset()` | `void` | Reset for re-recording |

### Resetting

```cpp
// Reset individual buffer (pool must have eResetCommandBuffer)
cmd.reset();

// Or reset entire pool
pool->reset();
```

## CommandBufferRecorder

RAII wrapper that handles begin/end automatically:

```cpp
{
    CommandBufferRecorder recorder(cmd);
    // vkBeginCommandBuffer called in constructor

    // Record commands...
    recorder.bindPipeline(/*...*/);

}  // vkEndCommandBuffer called in destructor
```

### With Usage Flags

```cpp
// One-time submit
CommandBufferRecorder recorder(cmd,
    vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

// Simultaneous use (can be pending on multiple queues)
CommandBufferRecorder recorder(cmd,
    vk::CommandBufferUsageFlagBits::eSimultaneousUse);
```

## Recording Commands

### Pipeline Binding

```cpp
recorder.bindPipeline(vk::PipelineBindPoint::eGraphics,
                      pipeline->handle());
```

### Descriptor Sets

```cpp
recorder.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    layout.handle(),
    0,                    // First set
    {descriptorSet},      // Sets
    {dynamicOffset}       // Dynamic offsets
);
```

### Vertex/Index Buffers

```cpp
// Vertex buffer
recorder.bindVertexBuffers(0, {vertexBuffer->handle()}, {0});

// Index buffer
recorder.bindIndexBuffer(indexBuffer->handle(), 0,
                         vk::IndexType::eUint32);
```

### Drawing

```cpp
// Non-indexed
recorder.draw(vertexCount, instanceCount, firstVertex, firstInstance);

// Indexed
recorder.drawIndexed(indexCount, instanceCount, firstIndex,
                     vertexOffset, firstInstance);
```

### Push Constants

```cpp
recorder.pushConstants(
    layout.handle(),
    vk::ShaderStageFlagBits::eVertex,
    0,                     // Offset
    sizeof(pushData),
    &pushData
);
```

### Viewport/Scissor

```cpp
vk::Viewport viewport{0, 0, width, height, 0, 1};
recorder.setViewport(0, viewport);

vk::Rect2D scissor{{0, 0}, {width, height}};
recorder.setScissor(0, scissor);
```

## Dynamic Rendering

Vulkan 1.3 dynamic rendering (no render pass objects):

```cpp
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = colorView->handle(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}}
};

vk::RenderingInfo renderInfo{
    .renderArea = {{0, 0}, {width, height}},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .pDepthAttachment = &depthAttachment
};

cmd.handle().beginRendering(renderInfo);
// Draw calls here
cmd.handle().endRendering();
```

## Barriers

Using Synchronization2:

```cpp
vk::ImageMemoryBarrier2 barrier{
    .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
    .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
    .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .image = image->handle(),
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
};

vk::DependencyInfo depInfo{
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
};

cmd.handle().pipelineBarrier2(depInfo);
```

## Submission

```cpp
vk::CommandBufferSubmitInfo cmdInfo{
    .commandBuffer = cmd.handle()
};

vk::SubmitInfo2 submitInfo{
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cmdInfo
};

device->graphics_queue().submit(submitInfo, fence.handle());
```

## Best Practices

1. **Use RAII recorder** for automatic begin/end
2. **Batch commands** to reduce submit overhead
3. **Use secondary buffers** for reusable sequences
4. **Minimize state changes** between draws
5. **Use appropriate barriers** for synchronization
