---
sidebar_position: 2
title: "Device & GPU Selection"
---

# Device & GPU Selection

The `DeviceFinder` selects a physical GPU from the system and creates a logical `Device` with the features your application needs. The `Device` is the central object through which you access queues, allocate resources, and record commands.

## Design Rationale

Vulkan separates the concept of a *physical device* (the actual GPU hardware) from a *logical device* (your application's handle to that GPU with specific features enabled). Selecting the right GPU and enabling the right features in raw Vulkan requires querying device properties, checking extension support, finding queue families, and filling out multiple chained feature structures -- often hundreds of lines of boilerplate.

`DeviceFinder` provides a declarative API: you state *what you need*, and it finds a GPU that satisfies all requirements and creates a device with exactly those features enabled. If no GPU meets the requirements, `build()` throws a clear error rather than producing a broken device.

The `Device` class then provides access to queues (graphics, presentation) through named methods, so you never have to deal with queue family indices directly.

## API Reference

### DeviceFinder

```cpp
namespace vw {

class DeviceFinder {
public:
    DeviceFinder(std::span<PhysicalDevice> physicalDevices) noexcept;

    DeviceFinder &with_queue(vk::QueueFlags queueFlags);
    DeviceFinder &with_presentation(vk::SurfaceKHR surface) noexcept;
    DeviceFinder &with_synchronization_2() noexcept;
    DeviceFinder &with_ray_tracing() noexcept;
    DeviceFinder &with_dynamic_rendering() noexcept;
    DeviceFinder &with_descriptor_indexing() noexcept;
    DeviceFinder &with_scalar_block_layout() noexcept;

    std::shared_ptr<Device> build();
    std::optional<PhysicalDevice> get() noexcept;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/DeviceFinder.h`

You do not construct `DeviceFinder` directly. Instead, call `instance->findGpu()`, which returns a `DeviceFinder` pre-populated with the system's available GPUs:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```

Every `with_*` method returns a reference to the same `DeviceFinder`, enabling method chaining. Each call adds a requirement -- GPUs that do not meet a requirement are eliminated from consideration.

#### Queue Requirements

##### `with_queue(vk::QueueFlags queueFlags)`

Requires a queue family that supports the specified flags. Common flags:

| Flag | Purpose |
|------|---------|
| `vk::QueueFlagBits::eGraphics` | Rendering commands (draw, clear, blit) |
| `vk::QueueFlagBits::eCompute` | Compute shader dispatch |
| `vk::QueueFlagBits::eTransfer` | Memory transfer operations |

A graphics queue implicitly supports transfer and compute operations, so in most cases you only need to request `eGraphics`. You can call `with_queue()` multiple times to request queues of different types.

##### `with_presentation(vk::SurfaceKHR surface)`

Requires a queue family capable of presenting to the given surface. This is needed for windowed applications that display rendered images on screen. The surface handle is obtained from `window.create_surface(*instance)`:

```cpp
auto surface = window.create_surface(*instance);
auto device = instance->findGpu()
    .with_presentation(surface.handle())
    // ...
    .build();
```

If you are doing offscreen rendering (headless, tests), omit this call.

#### Feature Requirements

##### `with_synchronization_2()`

Enables Vulkan 1.3 synchronization2 features (`VkPhysicalDeviceSynchronization2Features`). This is **required for VulkanWrapper's barrier system** (`ResourceTracker` in namespace `vw::Barrier`), which uses `vk::PipelineStageFlagBits2` and `vk::AccessFlags2` instead of the legacy v1 types.

You should always enable this -- the library's anti-patterns explicitly prohibit the v1 synchronization types.

##### `with_dynamic_rendering()`

Enables Vulkan 1.3 dynamic rendering features (`VkPhysicalDeviceDynamicRenderingFeatures`). This is **required** because VulkanWrapper uses `cmd.beginRendering()` / `cmd.endRendering()` instead of legacy render passes (`VkRenderPass` / `VkFramebuffer`).

You should always enable this.

##### `with_ray_tracing()`

Enables ray tracing extensions: acceleration structures (`VK_KHR_acceleration_structure`), ray query (`VK_KHR_ray_query`), and ray tracing pipelines (`VK_KHR_ray_tracing_pipeline`). Only request this if your application uses ray tracing features from the `vw::rt` namespace.

Not all GPUs support ray tracing. On systems without support (especially integrated GPUs), this requirement will cause `build()` to throw. Use `get()` to probe before committing.

##### `with_descriptor_indexing()`

Enables descriptor indexing (bindless resources) from Vulkan 1.2 (`VkPhysicalDeviceVulkan12Features`). This allows arrays of descriptors indexed dynamically in shaders, which is essential for the bindless material system in `vw::Model::Material`.

##### `with_scalar_block_layout()`

Enables scalar block layout from Vulkan 1.2. This allows shader storage buffers to use C-like scalar alignment instead of std430 rules, simplifying data layout between C++ structs and GLSL.

#### Building

##### `build()`

Selects the best GPU that meets all requirements and creates a logical device. Returns `std::shared_ptr<Device>`. Throws if no suitable GPU is found.

The selection process:

1. Filters out GPUs that lack required extensions, queue families, or features.
2. Among remaining candidates, prefers discrete GPUs over integrated ones.
3. Creates the device with all requested features enabled and the appropriate extensions loaded.

##### `get()`

Finds a suitable physical device **without** creating a logical device. Returns `std::optional<PhysicalDevice>` -- `std::nullopt` if no GPU meets the requirements. This is useful for probing hardware capabilities before committing to device creation:

```cpp
auto finder = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_ray_tracing();

auto physical = finder.get();
if (!physical) {
    // Ray tracing not available -- fall back
}
```

### Device

```cpp
namespace vw {

class Device {
public:
    Queue &graphicsQueue();
    [[nodiscard]] const PresentQueue &presentQueue() const;
    void wait_idle() const;
    [[nodiscard]] vk::PhysicalDevice physical_device() const;
    [[nodiscard]] vk::Device handle() const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Device.h`

`Device` is non-copyable and non-movable. It is always managed through a `shared_ptr` returned by `DeviceFinder::build()`.

#### `graphicsQueue()`

Returns a reference to the graphics `Queue`. This is where you submit command buffers for rendering. The queue is created automatically based on the `with_queue()` call during device finding.

```cpp
auto &queue = device->graphicsQueue();
queue.enqueue_command_buffer(cmd);
auto fence = queue.submit({}, {}, {});
fence.wait();
```

#### `presentQueue()`

Returns a const reference to the `PresentQueue`. Only available if `with_presentation()` was called during GPU selection. Attempting to access the present queue on a device created without presentation support leads to undefined behavior.

```cpp
device->presentQueue().present(swapchain, imageIndex, renderFinished);
```

#### `wait_idle()`

Blocks the CPU until all previously submitted GPU work completes. This is a heavy synchronization point. Use it:

- Before destroying resources (cleanup at application exit).
- Before recreating the swapchain.
- During debugging when you need to inspect GPU results.

Do **not** use it in a render loop -- it stalls the pipeline completely.

#### `physical_device()`

Returns the underlying `vk::PhysicalDevice` handle. Useful for querying device properties, limits, or format support that VulkanWrapper does not expose directly:

```cpp
auto props = device->physical_device().getProperties();
```

#### `handle()`

Returns the raw `vk::Device` handle. Needed when interacting with raw Vulkan calls or creating objects that VulkanWrapper does not wrap.

### Queue

```cpp
namespace vw {

class Queue {
public:
    void enqueue_command_buffer(vk::CommandBuffer command_buffer);
    void enqueue_command_buffers(std::span<const vk::CommandBuffer> command_buffers);

    Fence submit(std::span<const vk::PipelineStageFlags> waitStage,
                 std::span<const vk::Semaphore> waitSemaphores,
                 std::span<const vk::Semaphore> signalSemaphores);
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Queue.h`

#### `enqueue_command_buffer()` / `enqueue_command_buffers()`

Adds one or more command buffers to the queue's pending list. The command buffers are **not** submitted to the GPU until `submit()` is called. This lets you batch multiple command buffers into a single submit for efficiency.

#### `submit()`

Submits all enqueued command buffers to the GPU and returns a `Fence` you can wait on. The three parameters control synchronization:

- **waitStage / waitSemaphores:** The GPU waits on each semaphore at the corresponding pipeline stage before executing commands. Pass empty spans `{}` if there is nothing to wait on.
- **signalSemaphores:** The GPU signals these semaphores after all commands complete. Pass an empty span `{}` if you do not need to signal anything.

The returned `Fence` lets you know when the GPU finishes:

```cpp
auto fence = queue.submit({}, {}, {});
fence.wait();  // Block until GPU completes
```

### PresentQueue

```cpp
namespace vw {

class PresentQueue {
public:
    void present(const Swapchain &swapchain, uint32_t imageIndex,
                 const Semaphore &waitSemaphore) const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/PresentQueue.h`

The dedicated queue for presentation. Obtained via `device->presentQueue()`. In many GPUs, the present queue is the same physical queue as the graphics queue, but Vulkan treats them as logically separate.

The `present()` method waits on the given semaphore (which should be signaled by your rendering commands) and then presents the specified swapchain image to the display.

## Usage Patterns

### Offscreen Rendering (No Window)

For headless rendering, tests, or compute workloads:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```

### Windowed Application

For applications that render to a window:

```cpp
auto surface = window.create_surface(*instance);

auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_presentation(surface.handle())
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```

### Ray Tracing Application

For applications that use hardware ray tracing:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .with_ray_tracing()
    .with_descriptor_indexing()
    .with_scalar_block_layout()
    .build();
```

### Checking Ray Tracing Support Before Committing

Use `get()` to probe without creating a device:

```cpp
auto finder = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_ray_tracing();

auto physical = finder.get();
if (!physical) {
    // Fall back to rasterization-only path
}
```

This pattern is used in ray tracing tests:

```cpp
auto *gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```

### Submitting Work and Waiting

```cpp
auto &queue = device->graphicsQueue();
queue.enqueue_command_buffer(cmd);

// Submit with semaphore synchronization
auto fence = queue.submit(
    {vk::PipelineStageFlagBits::eColorAttachmentOutput},  // wait stages
    {imageAvailable.handle()},                              // wait semaphores
    {renderFinished.handle()});                             // signal semaphores

fence.wait();
```

## Common Pitfalls

### Forgetting `with_synchronization_2()` or `with_dynamic_rendering()`

**Symptom:** Validation errors about features not being enabled when using `ResourceTracker` or `cmd.beginRendering()`.

**Cause:** These Vulkan 1.3 features must be explicitly requested at device creation time even though the API version is 1.3. The API version makes the *extensions* available as core, but the *features* still need opt-in.

**Fix:** Always include both:

```cpp
instance->findGpu()
    .with_synchronization_2()
    .with_dynamic_rendering()
    // ...
    .build();
```

### Requesting `with_ray_tracing()` on unsupported hardware

**Symptom:** `build()` throws because no GPU meets the requirements.

**Cause:** Many GPUs (especially integrated ones and software renderers) do not support ray tracing extensions.

**Fix:** Use `get()` to check first, or catch the exception and fall back gracefully. In tests, use the `GTEST_SKIP()` pattern shown above.

### Accessing `presentQueue()` without `with_presentation()`

**Symptom:** Crash or undefined behavior when calling `device->presentQueue()`.

**Cause:** The present queue is only created if `with_presentation(surface)` was called during device setup.

**Fix:** Always call `with_presentation()` when building a windowed application:

```cpp
auto surface = window.create_surface(*instance);
auto device = instance->findGpu()
    .with_presentation(surface.handle())
    // ...
    .build();
```

### Forgetting to call `fence.wait()` before reusing resources

**Symptom:** Rendering corruption, validation errors about command buffers still in use.

**Cause:** GPU execution is asynchronous. After `queue.submit()`, the GPU may still be processing commands while the CPU continues. If you overwrite a buffer or command buffer that the GPU is still reading, you get a data race.

**Fix:** Wait on the fence returned by `submit()` before reusing any resources involved in that submission:

```cpp
auto fence = queue.submit({}, {}, {});
fence.wait();
// Now safe to modify buffers, rerecord command buffers, etc.
```

## Integration with Other Modules

| Next Step | How | See |
|-----------|-----|-----|
| Memory allocator | `AllocatorBuilder(instance, device).build()` | [Allocator](../memory/allocator.md) |
| Image views | `ImageViewBuilder(device, image)` | [Image Views](../image/image-views.md) |
| Samplers | `SamplerBuilder(device)` | [Samplers](../image/samplers.md) |
| Swapchain | `window.create_swapchain(device, surface.handle())` | [Swapchain & Window](swapchain.md) |
| Command pools | `CommandPoolBuilder(device).build()` | Command module |
