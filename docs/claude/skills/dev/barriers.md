# Barriers

Use `ResourceTracker` (`Synchronization/ResourceTracker.h`, namespace `vw::Barrier`) for all synchronization. Never use raw `vkCmdPipelineBarrier`.

Note: `Memory/Barrier.h` contains legacy standalone helpers (`execute_image_barrier_*`). Prefer `ResourceTracker` for new code.

## Basic Pattern

```cpp
vw::Barrier::ResourceTracker tracker;

// Track current state
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});

// Request new state
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});

// Generate barriers
tracker.flush(cmd);
```

## Common Transitions

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

**After TLAS Build -> Ray Tracing:**
```cpp
tracker.track({tlas, eAccelerationStructureBuildKHR, eAccelerationStructureWriteKHR});
tracker.request({tlas, eRayTracingShaderKHR, eAccelerationStructureReadKHR});
```

## Transfer API

`Transfer` (`Memory/Transfer.h`, namespace `vw`) wraps copy operations with an embedded `ResourceTracker` for automatic barrier management.

```cpp
vw::Transfer transfer;

// Blit image (automatic barrier management)
transfer.blit(cmd, srcImage, dstImage);
transfer.blit(cmd, srcImage, dstImage, srcSubresource, dstSubresource, vk::Filter::eLinear);

// Copy buffer (srcOffset, dstOffset, then size)
transfer.copyBuffer(cmd, src, dst, srcOffset, dstOffset, size);

// Copy between buffer and image
transfer.copyBufferToImage(cmd, srcBuffer, dstImage, srcOffset);
transfer.copyImageToBuffer(cmd, srcImage, dstBuffer, dstOffset);

// Save GPU image to disk (handles staging, format conversion, submission)
transfer.saveToFile(cmd, allocator, queue, image, "output.png");

// Access embedded tracker for manual state management
auto& tracker = transfer.resourceTracker();
```
