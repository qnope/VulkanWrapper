---
sidebar_position: 3
---

# Swapchain

The `Swapchain` class manages the presentation chain for displaying rendered images to a window.

## Overview

```cpp
#include <VulkanWrapper/Vulkan/Swapchain.h>

auto swapchain = SwapchainBuilder()
    .setDevice(device)
    .setSurface(surface)
    .setExtent(Width{1280}, Height{720})
    .setPresentMode(vk::PresentModeKHR::eMailbox)
    .build();
```

## SwapchainBuilder

### Methods

| Method | Description |
|--------|-------------|
| `setDevice(shared_ptr<Device>)` | Set the logical device |
| `setSurface(shared_ptr<Surface>)` | Set the window surface |
| `setExtent(Width, Height)` | Set swapchain dimensions |
| `setPresentMode(vk::PresentModeKHR)` | Set presentation mode |
| `setImageCount(uint32_t)` | Set number of images |
| `setFormat(vk::SurfaceFormatKHR)` | Set surface format |
| `build()` | Create the swapchain |

### Example

```cpp
auto swapchain = SwapchainBuilder()
    .setDevice(device)
    .setSurface(surface)
    .setExtent(Width{window->width()}, Height{window->height()})
    .setPresentMode(vk::PresentModeKHR::eFifo)
    .setImageCount(3)
    .build();
```

## Swapchain Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::SwapchainKHR` | Get the raw handle |
| `images()` | `span<const vk::Image>` | Get swapchain images |
| `image_views()` | `span<const ImageView>` | Get image views |
| `format()` | `vk::Format` | Get image format |
| `extent()` | `vk::Extent2D` | Get dimensions |
| `image_count()` | `uint32_t` | Get number of images |
| `acquire_next_image(Semaphore&)` | `uint32_t` | Acquire next image |

### Acquiring Images

```cpp
uint32_t imageIndex = swapchain->acquire_next_image(imageAvailable);
```

### Presenting

```cpp
vk::PresentInfoKHR presentInfo{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderFinished.handle(),
    .swapchainCount = 1,
    .pSwapchains = &swapchain->handle(),
    .pImageIndices = &imageIndex
};

device->present_queue().present(presentInfo);
```

## Present Modes

| Mode | Description | V-Sync |
|------|-------------|--------|
| `eImmediate` | No waiting, may tear | No |
| `eFifo` | Wait for V-blank | Yes |
| `eFifoRelaxed` | V-sync with late frame tolerance | Partial |
| `eMailbox` | Triple buffering, no tearing | Yes |

### Choosing a Present Mode

```cpp
// V-Sync (guaranteed supported)
.setPresentMode(vk::PresentModeKHR::eFifo)

// Low latency (if supported)
.setPresentMode(vk::PresentModeKHR::eMailbox)

// Maximum FPS (may tear)
.setPresentMode(vk::PresentModeKHR::eImmediate)
```

## Surface

The `Surface` class wraps the window surface:

```cpp
#include <VulkanWrapper/Vulkan/Surface.h>

// Create from window
auto surface = window->create_surface(instance);

// Get capabilities
auto capabilities = surface->capabilities(device->physical_device());
auto formats = surface->formats(device->physical_device());
auto presentModes = surface->present_modes(device->physical_device());
```

## Window Integration

### Creating a Surface

```cpp
// SDL3 window creates the surface
auto window = WindowBuilder()
    .setWidth(1280)
    .setHeight(720)
    .setTitle("My App")
    .build();

auto surface = window->create_surface(instance);
```

### Getting Extensions

```cpp
// Get required instance extensions for the window system
auto extensions = window->vulkan_extensions();

auto instance = InstanceBuilder()
    .addExtensions(extensions)
    .build();
```

## Resize Handling

Swapchains need to be recreated when the window resizes:

```cpp
void onResize(uint32_t width, uint32_t height) {
    device->wait_idle();

    swapchain = SwapchainBuilder()
        .setDevice(device)
        .setSurface(surface)
        .setExtent(Width{width}, Height{height})
        .setPresentMode(presentMode)
        .build();
}
```

## Render Loop

Typical render loop structure:

```cpp
while (running) {
    // Handle events...

    // Acquire image
    uint32_t imageIndex = swapchain->acquire_next_image(imageAvailable);

    // Record commands
    auto& cmd = commandBuffers[imageIndex];
    // ... render to swapchain image ...

    // Submit
    vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd.handle()};
    vk::SemaphoreSubmitInfo waitInfo{
        .semaphore = imageAvailable.handle(),
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    vk::SemaphoreSubmitInfo signalInfo{
        .semaphore = renderFinished.handle()
    };

    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalInfo
    };

    device->graphics_queue().submit(submitInfo, fence.handle());

    // Present
    vk::PresentInfoKHR presentInfo{/* ... */};
    device->present_queue().present(presentInfo);
}
```

## Best Practices

1. **Use FIFO as fallback** - it's always supported
2. **Prefer Mailbox** for low-latency triple buffering
3. **Handle resize gracefully** by recreating the swapchain
4. **Wait for device idle** before recreating
5. **Use at least 2-3 images** for smooth rendering
