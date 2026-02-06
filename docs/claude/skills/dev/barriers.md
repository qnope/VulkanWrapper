# Barriers

Use `ResourceTracker` (`Synchronization/ResourceTracker.h`, namespace `vw::Barrier`) for all synchronization. Never use raw `vkCmdPipelineBarrier`.

Note: `Memory/Barrier.h` contains legacy standalone helpers (`execute_image_barrier_*`). Prefer `ResourceTracker` for new code.

## Resource State Types

```cpp
// All in namespace vw::Barrier
struct ImageState {
    vk::Image image;
    vk::ImageSubresourceRange range;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct BufferState {
    vk::Buffer buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct AccelerationStructureState {
    vk::AccelerationStructureKHR as;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

using ResourceState = std::variant<ImageState, BufferState, AccelerationStructureState>;
```

## Basic Pattern

```cpp
vw::Barrier::ResourceTracker tracker;

// 1. Track current state
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});

// 2. Request new state
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});

// 3. Generate and submit barriers
tracker.flush(cmd);
```

## Common Image Transitions

**Undefined -> Color Attachment:**
```cpp
tracker.track({image, range, eUndefined, eTopOfPipe, eNone});
tracker.request({image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
```

**Color Attachment -> Shader Read:**
```cpp
tracker.track({image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.request({image, range, eShaderReadOnlyOptimal, eFragmentShader, eShaderSampledRead});
```

**Transfer Dst -> Shader Read (after staging upload):**
```cpp
tracker.track({image, range, eTransferDstOptimal, eTransfer, eTransferWrite});
tracker.request({image, range, eShaderReadOnlyOptimal, eFragmentShader, eShaderSampledRead});
```

**Shader Write -> Shader Read (storage image):**
```cpp
tracker.track({image, range, eGeneral, eComputeShader, eShaderStorageWrite});
tracker.request({image, range, eShaderReadOnlyOptimal, eFragmentShader, eShaderSampledRead});
```

## Common Buffer Transitions

**After staging upload -> Vertex shader read:**
```cpp
tracker.track(BufferState{buffer, 0, VK_WHOLE_SIZE, eTransfer, eTransferWrite});
tracker.request(BufferState{buffer, 0, VK_WHOLE_SIZE, eVertexInput, eVertexAttributeRead});
```

**After compute write -> Fragment shader read:**
```cpp
tracker.track(BufferState{buffer, 0, VK_WHOLE_SIZE, eComputeShader, eShaderStorageWrite});
tracker.request(BufferState{buffer, 0, VK_WHOLE_SIZE, eFragmentShader, eShaderStorageRead});
```

**After compute write -> Compute read (multi-pass compute):**
```cpp
tracker.track(BufferState{buffer, 0, VK_WHOLE_SIZE, eComputeShader, eShaderStorageWrite});
tracker.request(BufferState{buffer, 0, VK_WHOLE_SIZE, eComputeShader, eShaderStorageRead});
```

**After compute write -> Indirect draw read:**
```cpp
tracker.track(BufferState{buffer, 0, VK_WHOLE_SIZE, eComputeShader, eShaderStorageWrite});
tracker.request(BufferState{buffer, 0, VK_WHOLE_SIZE, eDrawIndirect, eIndirectCommandRead});
```

## Acceleration Structure Transitions

**After TLAS Build -> Ray Tracing:**
```cpp
tracker.track({tlas, eAccelerationStructureBuildKHR, eAccelerationStructureWriteKHR});
tracker.request({tlas, eRayTracingShaderKHR, eAccelerationStructureReadKHR});
```

**After TLAS Build -> Compute Shader read:**
```cpp
tracker.track({tlas, eAccelerationStructureBuildKHR, eAccelerationStructureWriteKHR});
tracker.request({tlas, eComputeShader, eAccelerationStructureReadKHR});
```

## Transfer API

`Transfer` (`Memory/Transfer.h`) wraps copy operations with an embedded `ResourceTracker`:

```cpp
vw::Transfer transfer;

transfer.blit(cmd, srcImage, dstImage);
transfer.copyBuffer(cmd, src, dst, srcOffset, dstOffset, size);
transfer.copyBufferToImage(cmd, srcBuffer, dstImage, srcOffset);
transfer.copyImageToBuffer(cmd, srcImage, dstBuffer, dstOffset);
transfer.saveToFile(cmd, allocator, queue, image, "output.png");

// Access embedded tracker for manual state management
auto& tracker = transfer.resourceTracker();
```

## Important Notes

- Always use `vk::PipelineStageFlagBits2` and `vk::AccessFlags2` (Synchronization2), never v1 flags
- `DescriptorSet` carries `vector<Barrier::ResourceState>` -- use `descriptor_set.resources()` to get states to track
- `ResourceTracker` supports interval-based tracking for buffer subranges (partial buffer barriers)
- Multiple `track()` + `request()` calls can be batched before a single `flush()` -- this minimizes barrier commands
