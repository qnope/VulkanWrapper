---
sidebar_position: 1
---

# Fences & Semaphores

VulkanWrapper provides RAII wrappers for the two fundamental Vulkan
synchronization primitives: **Fences** (CPU-GPU sync) and **Semaphores**
(GPU-GPU sync). This page explains what each one does, how to create them,
and how to use them together for frame synchronization.

## CPU-GPU vs. GPU-GPU Synchronization

Before diving into the API, it helps to understand *when* you need each
primitive:

| Primitive     | Waiter | Signaler | Typical Use                    |
|---------------|--------|----------|--------------------------------|
| **Fence**     | CPU    | GPU      | "Wait until the GPU finishes this submit" |
| **Semaphore** | GPU    | GPU      | "Don't start rendering until the swapchain image is ready" |

- A **Fence** lets your C++ code block until the GPU signals it. Use fences
  to know when a command buffer has finished so you can safely reuse its
  resources.
- A **Semaphore** coordinates work *within* the GPU -- between queue submits,
  or between a submit and a present operation. Your CPU code never waits on a
  semaphore.

## Fence

### How Fences Are Created

`FenceBuilder` has a **private** constructor -- you do not create fences
directly. Instead, `Queue::submit()` creates and returns a `Fence`
automatically:

```cpp
// Enqueue a command buffer, then submit.
// submit() creates an unsignaled fence internally, submits the work,
// and returns the fence so you can wait on it.
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});

// Block the CPU until this submit completes
fence.wait();
```

The `submit` signature:

```cpp
Fence Queue::submit(
    std::span<const vk::PipelineStageFlags> waitStages,
    std::span<const vk::Semaphore> waitSemaphores,
    std::span<const vk::Semaphore> signalSemaphores);
```

### Fence Methods

| Method    | Description                                   |
|-----------|-----------------------------------------------|
| `wait()`  | Block the CPU until the GPU signals this fence |
| `reset()` | Reset the fence to unsignaled state            |
| `handle()`| Get the underlying `vk::Fence` handle          |

### Typical Fence Usage

```cpp
// Record commands
auto cmd = commandPool.allocate_command_buffer();
{
    vw::CommandBufferRecorder recorder(cmd);
    // ... record draw calls ...
}

// Submit and get a fence
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});

// Wait for completion before reading results
fence.wait();
```

The fence is automatically destroyed when it goes out of scope (RAII via
`vk::UniqueFence`). It is move-only -- you cannot copy a fence.

## Semaphore

### SemaphoreBuilder

Unlike fences, semaphores are created explicitly through `SemaphoreBuilder`:

```cpp
auto semaphore = vw::SemaphoreBuilder(device).build();
```

The constructor takes a `std::shared_ptr<const Device>`.

### Semaphore Methods

| Method    | Description                              |
|-----------|------------------------------------------|
| `handle()`| Get the underlying `vk::Semaphore` handle |

Like fences, semaphores are RAII-managed and move-only.

### Using Semaphores with Queue Submit

Semaphores are passed to `Queue::submit()` as wait and signal parameters:

```cpp
// Create semaphores for frame synchronization
auto imageAvailable = vw::SemaphoreBuilder(device).build();
auto renderFinished = vw::SemaphoreBuilder(device).build();

// Acquire swapchain image -- signals imageAvailable when ready
uint32_t imageIndex = swapchain->acquire_next_image(imageAvailable);

// Submit rendering:
//   - wait on imageAvailable before color output stage
//   - signal renderFinished when done
vk::PipelineStageFlags waitStage =
    vk::PipelineStageFlagBits::eColorAttachmentOutput;

queue.enqueue_command_buffer(cmd);
auto fence = queue.submit(
    std::span(&waitStage, 1),
    std::span(&imageAvailable.handle(), 1),
    std::span(&renderFinished.handle(), 1));

// Present -- waits on renderFinished
// ...
```

## Frame Synchronization Pattern

Real applications render multiple frames concurrently ("frames in flight")
to keep the GPU busy while the CPU prepares the next frame. This requires
one set of synchronization objects per in-flight frame.

### Setup

```cpp
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

struct FrameSync {
    vw::Semaphore imageAvailable;
    vw::Semaphore renderFinished;
    std::optional<vw::Fence> inFlightFence;
};

// Create synchronization objects for each frame
std::array<FrameSync, FRAMES_IN_FLIGHT> frames;
for (auto &frame : frames) {
    frame.imageAvailable = vw::SemaphoreBuilder(device).build();
    frame.renderFinished = vw::SemaphoreBuilder(device).build();
    // inFlightFence starts as nullopt; it will be set by the
    // first Queue::submit() call.
}
```

### Render Loop

```cpp
uint32_t currentFrame = 0;

while (running) {
    auto &sync = frames[currentFrame];

    // Wait for this frame slot's previous work to finish.
    // On the first iteration, inFlightFence is nullopt so we skip.
    if (sync.inFlightFence) {
        sync.inFlightFence->wait();
    }

    // Acquire next swapchain image
    uint32_t imageIndex =
        swapchain->acquire_next_image(sync.imageAvailable);

    // Record commands for this frame...
    auto cmd = commandPool.allocate_command_buffer();
    {
        vw::CommandBufferRecorder recorder(cmd);
        // ... draw ...
    }

    // Submit
    vk::PipelineStageFlags waitStage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;

    queue.enqueue_command_buffer(cmd);
    sync.inFlightFence = queue.submit(
        std::span(&waitStage, 1),
        std::span(&sync.imageAvailable.handle(), 1),
        std::span(&sync.renderFinished.handle(), 1));

    // Present (waits on renderFinished)
    // ...

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
}

// Wait for all GPU work before cleanup
device->handle().waitIdle();
```

### Why "Frames in Flight"?

Without frames in flight, the CPU would record commands, submit, and then
block on the fence every single frame. The GPU would be idle while the CPU
records, and the CPU would be idle while the GPU renders. With two (or more)
frames in flight:

```
CPU:  [record F0] [record F1] [wait F0, record F0] [wait F1, record F1] ...
GPU:               [render F0] [render F1]          [render F0]          ...
```

The CPU and GPU overlap their work, maximizing throughput.

## Key Differences from Raw Vulkan

| Raw Vulkan                                  | VulkanWrapper                                   |
|---------------------------------------------|-------------------------------------------------|
| `vkCreateFence` / `vkDestroyFence`          | Automatic via `Queue::submit()` return + RAII   |
| `vkCreateSemaphore` / `vkDestroySemaphore`  | `SemaphoreBuilder(device).build()` + RAII       |
| `vkWaitForFences` with array + timeout      | `fence.wait()` (single fence, no timeout)       |
| `vkResetFences`                             | `fence.reset()`                                 |
| Manual lifetime management                  | Move-only RAII -- destruction is automatic       |

## Best Practices

1. **Let `Queue::submit()` create fences** -- do not try to construct
   `FenceBuilder` directly (its constructor is private).
2. **Always wait on a fence before reusing** the resources associated with
   that submission (command buffers, descriptor sets, etc.).
3. **Use semaphores** for acquire-render-present synchronization.
4. **Match frame sync object count** to `FRAMES_IN_FLIGHT` -- each in-flight
   frame needs its own semaphores and fence.
5. **Call `device->handle().waitIdle()`** before shutdown to ensure all GPU
   work is complete.
