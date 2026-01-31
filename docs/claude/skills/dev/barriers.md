# Barrier Management with ResourceTracker

Use `ResourceTracker` for all synchronization barriers. Never use raw `vkCmdPipelineBarrier`.

## Basic Pattern

```cpp
#include "VulkanWrapper/Synchronization/ResourceTracker.h"

vw::Barrier::ResourceTracker tracker;

// 1. Track current state (what the resource IS)
tracker.track(vw::Barrier::ImageState{
    .image = image->handle(),
    .subresourceRange = image->full_range(),
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// 2. Request new state (what you NEED)
tracker.request(vw::Barrier::ImageState{
    .image = image->handle(),
    .subresourceRange = image->full_range(),
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});

// 3. Flush barriers to command buffer
tracker.flush(cmd);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Synchronization/ResourceTracker.h`

---

## Resource State Types

### ImageState

```cpp
vw::Barrier::ImageState{
    .image = vkImage,                    // vk::Image handle
    .subresourceRange = vk::ImageSubresourceRange{
        vk::ImageAspectFlagBits::eColor, // aspectMask
        0,                                // baseMipLevel
        VK_REMAINING_MIP_LEVELS,         // levelCount
        0,                                // baseArrayLayer
        VK_REMAINING_ARRAY_LAYERS        // layerCount
    },
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
};
```

### BufferState

```cpp
vw::Barrier::BufferState{
    .buffer = vkBuffer,                  // vk::Buffer handle
    .offset = 0,                         // Byte offset
    .size = VK_WHOLE_SIZE,               // Or specific size
    .stage = vk::PipelineStageFlagBits2::eVertexShader,
    .access = vk::AccessFlagBits2::eShaderStorageRead
};
```

### AccelerationStructureState

```cpp
vw::Barrier::AccelerationStructureState{
    .handle = asHandle,                  // vk::AccelerationStructureKHR
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
};
```

---

## Common Layout Transitions

### Undefined to Color Attachment

```cpp
// For rendering to a new/cleared image
tracker.track(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});
tracker.request(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});
```

### Color Attachment to Shader Read

```cpp
// After rendering, before sampling
tracker.track(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});
tracker.request(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
```

### Shader Read to Transfer Source

```cpp
// Before copy/blit operations
tracker.track(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
tracker.request(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eTransferSrcOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferRead
});
```

### Transfer Destination to Shader Read

```cpp
// After uploading texture data
tracker.track(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eTransferDstOptimal,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});
tracker.request(ImageState{
    .image = image, .subresourceRange = range,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
```

### Color Attachment to Present

```cpp
// Before presenting swapchain image
tracker.track(ImageState{
    .image = swapchainImage, .subresourceRange = range,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});
tracker.request(ImageState{
    .image = swapchainImage, .subresourceRange = range,
    .layout = vk::ImageLayout::ePresentSrcKHR,
    .stage = vk::PipelineStageFlagBits2::eBottomOfPipe,
    .access = vk::AccessFlagBits2::eNone
});
```

### Depth Attachment

```cpp
// For depth buffer
tracker.request(ImageState{
    .image = depthImage, .subresourceRange = depthRange,
    .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
             vk::PipelineStageFlagBits2::eLateFragmentTests,
    .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead |
              vk::AccessFlagBits2::eDepthStencilAttachmentWrite
});
```

---

## High-Level Transfer API

For common operations, use the `Transfer` class:

```cpp
#include "VulkanWrapper/Memory/Transfer.h"

vw::Transfer transfer;

// Blit with automatic barriers
transfer.blit(cmd, srcImage, dstImage);

// Copy buffer regions
transfer.copyBuffer(cmd, srcBuffer, dstBuffer, size, srcOffset, dstOffset);

// Save image to file (handles staging, layout transitions)
transfer.saveToFile(cmd, *allocator, queue, image, "output.png");

// Access underlying tracker for custom operations
transfer.resourceTracker().track(...);
transfer.resourceTracker().request(...);
transfer.resourceTracker().flush(cmd);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/Transfer.h`

---

## Buffer Barriers

```cpp
// Before compute shader reads buffer written by transfer
tracker.track(BufferState{
    .buffer = buffer, .offset = 0, .size = VK_WHOLE_SIZE,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});
tracker.request(BufferState{
    .buffer = buffer, .offset = 0, .size = VK_WHOLE_SIZE,
    .stage = vk::PipelineStageFlagBits2::eComputeShader,
    .access = vk::AccessFlagBits2::eShaderStorageRead
});

// Vertex buffer after staging upload
tracker.track(BufferState{
    .buffer = vertexBuffer, .offset = 0, .size = VK_WHOLE_SIZE,
    .stage = vk::PipelineStageFlagBits2::eTransfer,
    .access = vk::AccessFlagBits2::eTransferWrite
});
tracker.request(BufferState{
    .buffer = vertexBuffer, .offset = 0, .size = VK_WHOLE_SIZE,
    .stage = vk::PipelineStageFlagBits2::eVertexInput,
    .access = vk::AccessFlagBits2::eVertexAttributeRead
});
```

---

## Acceleration Structure Barriers

### After BLAS Build

```cpp
// BLAS build completes, prepare for TLAS build
tracker.track(AccelerationStructureState{
    .handle = blas,
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR
});
tracker.request(AccelerationStructureState{
    .handle = blas,
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
```

### After TLAS Build, Before Ray Tracing

```cpp
// TLAS build completes, prepare for ray tracing
tracker.track(AccelerationStructureState{
    .handle = tlas,
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR
});
tracker.request(AccelerationStructureState{
    .handle = tlas,
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
```

### RT Output Image to Shader Read

```cpp
// After ray tracing writes to storage image
tracker.track(ImageState{
    .image = rtOutput,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eGeneral,
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eShaderStorageWrite
});
tracker.request(ImageState{
    .image = rtOutput,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
```

### RT Shadow Query

```cpp
// Before using TLAS in any-hit shader for shadow rays
tracker.track(AccelerationStructureState{
    .handle = tlas,
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
// No transition needed if already in read state
```

### Complete RT Pipeline Sequence

```cpp
// 1. Build BLAS (initial or rebuild)
cmd.buildAccelerationStructuresKHR(blasBuildInfos, blasRanges);

// 2. Barrier: BLAS write -> TLAS build read
tracker.track(AccelerationStructureState{blasHandle, eBuild, eWrite});
tracker.request(AccelerationStructureState{blasHandle, eBuild, eRead});
tracker.flush(cmd);

// 3. Build TLAS
cmd.buildAccelerationStructuresKHR(tlasBuildInfo, tlasRange);

// 4. Barrier: TLAS write -> RT shader read
tracker.track(AccelerationStructureState{tlasHandle, eBuild, eWrite});
tracker.request(AccelerationStructureState{tlasHandle, eRTShader, eRead});
tracker.flush(cmd);

// 5. Trace rays
cmd.traceRaysKHR(raygen, miss, hit, callable, width, height, 1);

// 6. Barrier: RT output -> fragment shader read
tracker.track(ImageState{output, eGeneral, eRTShader, eStorageWrite});
tracker.request(ImageState{output, eShaderReadOnly, eFragment, eSampledRead});
tracker.flush(cmd);
```

---

## Fine-Grained Tracking

ResourceTracker uses `IntervalSet` for sub-resource tracking:

```cpp
// Track different mip levels with different states
tracker.track(ImageState{
    .image = image,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}, // Mip 0
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    ...
});
tracker.track(ImageState{
    .image = image,
    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 1, 3, 0, 1}, // Mips 1-3
    .layout = vk::ImageLayout::eTransferDstOptimal,
    ...
});
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/IntervalSet.h`

---

## Anti-Patterns

```cpp
// DON'T: Old barrier API
cmd.pipelineBarrier(
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::PipelineStageFlagBits::eFragmentShader,
    {}, {}, {}, imageBarrier
);

// DO: Synchronization2 via ResourceTracker
tracker.request(state);
tracker.flush(cmd);

// DON'T: Manual barrier construction
vk::ImageMemoryBarrier2 barrier{...};
vk::DependencyInfo depInfo{...};
cmd.pipelineBarrier2(depInfo);

// DO: Let ResourceTracker generate optimal barriers
tracker.track(currentState);
tracker.request(newState);
tracker.flush(cmd);  // Generates minimal barrier set

// DON'T: Forget to flush
tracker.track(state1);
tracker.request(state2);
// Missing flush!

// DO: Always flush before using the resource
tracker.flush(cmd);
cmd.bindDescriptorSets(...);  // Now safe to use
```
