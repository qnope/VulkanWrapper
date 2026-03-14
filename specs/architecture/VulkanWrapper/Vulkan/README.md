# Vulkan Module

`vw` namespace · `Vulkan/` directory · Tier 2

Core Vulkan object wrappers for instance, device, queues, and presentation. All created through builders, owned via RAII.

## Instance / InstanceBuilder

```cpp
auto instance = InstanceBuilder()
    .setDebug()                    // enable validation layers + debug messenger
    .setApiVersion(ApiVersion::e13)
    .addPortability()              // macOS portability subset
    .addExtensions(window.get_required_instance_extensions())
    .build();                      // returns shared_ptr<Instance>
```

## DeviceFinder

Fluent GPU selection, started from `instance->findGpu()`:

```cpp
auto device = instance->findGpu()
    .with_queue(eGraphics | eCompute | eTransfer)
    .with_presentation(surface.handle())
    .with_synchronization_2()
    .with_ray_tracing()
    .with_dynamic_rendering()
    .with_descriptor_indexing()
    .with_scalar_block_layout()
    .build();  // returns shared_ptr<Device>
```

Automatically filters physical devices by capability and selects the best match. Enables required extensions and features.

## Device

Wraps `vk::UniqueDevice`. Key methods:
- `graphicsQueue()` → `Queue&` for command submission
- `presentQueue()` → `PresentQueue` for swapchain presentation
- `wait_idle()` — device synchronization
- `physical_device()` → `vk::PhysicalDevice`

## Queue

- `enqueue_command_buffer(cmd)` — stage for submission
- `submit(waitStages, waitSemaphores, signalSemaphores)` → Fence

## Swapchain / SwapchainBuilder

```cpp
auto swapchain = SwapchainBuilder(device, surface, width, height)
    .with_old_swapchain(old.handle())  // for recreation
    .build();
auto index = swapchain.acquire_next_image(semaphore);
swapchain.present(index, renderFinished);
```

- `image_views()` — per-image views for rendering
- `number_images()` — image count for command buffer allocation

## Other Types

- **PhysicalDevice** — device properties, queue families, extension enumeration
- **Surface** — Vulkan surface handle (created from Window)
