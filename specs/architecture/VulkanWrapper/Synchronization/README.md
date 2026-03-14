# Synchronization Module

`vw::Barrier` namespace (ResourceTracker) · `vw` namespace (Fence, Semaphore) · `Synchronization/` directory · Tier 2-3

GPU synchronization primitives and automatic barrier management. Uses Vulkan Synchronization2 exclusively (`PipelineStageFlagBits2`, `AccessFlags2`).

## ResourceTracker (`vw::Barrier` namespace)

Automatic pipeline barrier management. Tracks resource states and generates optimal barriers:

```cpp
vw::Barrier::ResourceTracker tracker;
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.flush(cmd);  // emits vk::DependencyInfo with image/buffer/memory barriers
```

State types (combined in `ResourceState` variant):
- **ImageState** — image, subresource range, layout, stage, access
- **BufferState** — buffer, offset, size, stage, access
- **AccelerationStructureState** — handle, stage, access

Uses `IntervalSet` internally for sub-resource tracking — different mip levels or array layers can be in different states simultaneously.

**Anti-pattern**: Never use raw `vkCmdPipelineBarrier` — always use ResourceTracker.

## Fence / FenceBuilder · Semaphore / SemaphoreBuilder

RAII wrappers with builder pattern:

```cpp
auto fence = FenceBuilder(device).build();
auto semaphore = SemaphoreBuilder(device).build();
```

## Interval / IntervalSet

Half-open interval arithmetic for sub-resource tracking. `IntervalSet` manages disjoint sets of intervals (mip levels, buffer ranges) and supports split/merge operations. Used internally by ResourceTracker.
