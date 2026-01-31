---
sidebar_position: 1
---

# Fences and Semaphores

Synchronization primitives for CPU-GPU and GPU-GPU coordination.

## Overview

```cpp
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>

// CPU-GPU sync
auto fence = FenceBuilder()
    .setDevice(device)
    .setSignaled(true)  // Start signaled
    .build();

// GPU-GPU sync
auto semaphore = SemaphoreBuilder()
    .setDevice(device)
    .build();
```

## Fence

Fences synchronize the CPU with GPU work completion.

### FenceBuilder

| Method | Description |
|--------|-------------|
| `setDevice(device)` | Set the logical device |
| `setSignaled(bool)` | Start in signaled state |

### Fence Class

| Method | Description |
|--------|-------------|
| `handle()` | Get raw handle |
| `wait()` | Wait for signal (blocking) |
| `wait(timeout)` | Wait with timeout |
| `reset()` | Reset to unsignaled |
| `is_signaled()` | Check signal state |

### Usage

```cpp
// Create signaled fence (for first frame)
auto fence = FenceBuilder()
    .setDevice(device)
    .setSignaled(true)
    .build();

// Render loop
while (running) {
    // Wait for previous frame
    fence->wait();
    fence->reset();

    // Record and submit
    // ...

    device->graphics_queue().submit(submitInfo, fence->handle());
}
```

### Timeout

```cpp
// Wait up to 1 second
auto result = fence->wait(std::chrono::seconds(1));
if (result == vk::Result::eTimeout) {
    // Handle timeout
}
```

## Semaphore

Semaphores synchronize GPU operations across queues or submissions.

### SemaphoreBuilder

| Method | Description |
|--------|-------------|
| `setDevice(device)` | Set the logical device |

### Semaphore Class

| Method | Description |
|--------|-------------|
| `handle()` | Get raw handle |

### Usage

```cpp
auto imageAvailable = SemaphoreBuilder().setDevice(device).build();
auto renderFinished = SemaphoreBuilder().setDevice(device).build();

// Acquire swapchain image (signals imageAvailable)
uint32_t imageIndex = swapchain->acquire_next_image(*imageAvailable);

// Submit rendering (waits on imageAvailable, signals renderFinished)
vk::SemaphoreSubmitInfo waitInfo{
    .semaphore = imageAvailable->handle(),
    .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
};
vk::SemaphoreSubmitInfo signalInfo{
    .semaphore = renderFinished->handle()
};

vk::SubmitInfo2 submitInfo{
    .waitSemaphoreInfoCount = 1,
    .pWaitSemaphoreInfos = &waitInfo,
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cmdInfo,
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos = &signalInfo
};

// Present (waits on renderFinished)
vk::PresentInfoKHR presentInfo{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderFinished->handle(),
    .swapchainCount = 1,
    .pSwapchains = &swapchain->handle(),
    .pImageIndices = &imageIndex
};
```

## Frame Synchronization

Typical multi-frame setup:

```cpp
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

struct FrameSync {
    std::shared_ptr<Fence> inFlightFence;
    std::shared_ptr<Semaphore> imageAvailable;
    std::shared_ptr<Semaphore> renderFinished;
};

std::array<FrameSync, FRAMES_IN_FLIGHT> frameSync;

for (auto& frame : frameSync) {
    frame.inFlightFence = FenceBuilder()
        .setDevice(device)
        .setSignaled(true)
        .build();
    frame.imageAvailable = SemaphoreBuilder()
        .setDevice(device)
        .build();
    frame.renderFinished = SemaphoreBuilder()
        .setDevice(device)
        .build();
}
```

### Frame Loop

```cpp
uint32_t currentFrame = 0;

while (running) {
    auto& sync = frameSync[currentFrame];

    // Wait for this frame's previous work
    sync.inFlightFence->wait();
    sync.inFlightFence->reset();

    // Acquire image
    uint32_t imageIndex = swapchain->acquire_next_image(
        *sync.imageAvailable);

    // Submit
    submitInfo.pWaitSemaphoreInfos = /* imageAvailable */;
    submitInfo.pSignalSemaphoreInfos = /* renderFinished */;
    device->graphics_queue().submit(submitInfo,
                                    sync.inFlightFence->handle());

    // Present
    presentInfo.pWaitSemaphores = &sync.renderFinished->handle();
    device->present_queue().present(presentInfo);

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
}
```

## Best Practices

1. **Start fences signaled** for first frame
2. **Wait before reset** - ensure work is complete
3. **Use semaphores** for GPU-GPU sync
4. **Use fences** for CPU-GPU sync
5. **Match in-flight frames** to resource count
