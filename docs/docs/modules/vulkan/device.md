---
sidebar_position: 2
---

# Device

The `Device` class represents a logical Vulkan device, providing access to GPU capabilities and queues.

## Overview

```cpp
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>

auto device = instance->findGpu()
    .requireGraphicsQueue()
    .requireComputeQueue()
    .requirePresentQueue(surface)
    .requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    .find()
    .build();
```

## DeviceFinder

The `DeviceFinder` class provides a fluent API for GPU selection:

### Queue Requirements

```cpp
auto finder = instance->findGpu()
    .requireGraphicsQueue()      // Require graphics capability
    .requireComputeQueue()       // Require compute capability
    .requireTransferQueue()      // Require dedicated transfer
    .requirePresentQueue(surface);  // Require presentation
```

### Extension Requirements

```cpp
finder.requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
      .requireExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

// Ray tracing extensions (adds all required extensions)
finder.requireRayTracingExtensions();
```

### Feature Requirements

```cpp
finder.requireFeature(&vk::PhysicalDeviceFeatures::samplerAnisotropy)
      .requireFeature(&vk::PhysicalDeviceFeatures::multiDrawIndirect);
```

### Finding and Building

```cpp
auto deviceBuilder = finder.find();  // Selects best GPU
auto device = deviceBuilder.build(); // Creates logical device
```

## Device Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::Device` | Get the raw Vulkan handle |
| `physical_device()` | `const PhysicalDevice&` | Get physical device info |
| `graphics_queue()` | `Queue&` | Get graphics queue |
| `compute_queue()` | `Queue&` | Get compute queue |
| `transfer_queue()` | `Queue&` | Get transfer queue |
| `present_queue()` | `PresentQueue&` | Get presentation queue |
| `wait_idle()` | `void` | Wait for device to be idle |

### Queue Access

```cpp
auto& graphicsQueue = device->graphics_queue();
auto& computeQueue = device->compute_queue();
auto& presentQueue = device->present_queue();

// Queue properties
uint32_t familyIndex = graphicsQueue.family_index();
vk::Queue handle = graphicsQueue.handle();
```

### Waiting for Idle

```cpp
// Wait for all GPU work to complete
device->wait_idle();
```

## Queue Class

Represents a Vulkan queue:

```cpp
class Queue {
public:
    [[nodiscard]] vk::Queue handle() const noexcept;
    [[nodiscard]] uint32_t family_index() const noexcept;

    void submit(vk::SubmitInfo2 submitInfo,
                vk::Fence fence = nullptr) const;
    void submit(std::span<const vk::SubmitInfo2> submitInfos,
                vk::Fence fence = nullptr) const;
};
```

### Submitting Work

```cpp
vk::CommandBufferSubmitInfo cmdInfo{
    .commandBuffer = commandBuffer.handle()
};

vk::SubmitInfo2 submitInfo{
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cmdInfo
};

device->graphics_queue().submit(submitInfo, fence.handle());
```

## PresentQueue Class

Extends `Queue` with presentation capabilities:

```cpp
class PresentQueue : public Queue {
public:
    vk::Result present(vk::PresentInfoKHR presentInfo) const;
};
```

### Presenting

```cpp
vk::PresentInfoKHR presentInfo{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderFinished,
    .swapchainCount = 1,
    .pSwapchains = &swapchain,
    .pImageIndices = &imageIndex
};

auto result = device->present_queue().present(presentInfo);
```

## Physical Device

Access physical device properties:

```cpp
const auto& physical = device->physical_device();

// Device properties
auto properties = physical.properties();
std::cout << "GPU: " << properties.deviceName << '\n';

// Memory properties
auto memProps = physical.memory_properties();

// Format support
bool supported = physical.is_format_supported(
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageTiling::eOptimal,
    vk::FormatFeatureFlagBits::eSampledImage
);
```

## Ray Tracing Support

Check and enable ray tracing:

```cpp
auto device = instance->findGpu()
    .requireGraphicsQueue()
    .requireRayTracingExtensions()  // Adds all RT extensions
    .find()
    .build();

// This enables:
// - VK_KHR_acceleration_structure
// - VK_KHR_ray_tracing_pipeline
// - VK_KHR_deferred_host_operations
// - VK_KHR_buffer_device_address
// - And other dependencies
```

## Best Practices

1. **Request only needed queues** to avoid resource contention
2. **Check extension support** before requiring them
3. **Use `wait_idle()`** before destroying resources
4. **Cache queue references** rather than calling getters repeatedly
