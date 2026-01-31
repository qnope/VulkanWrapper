# Vulkan 1.3 Features

This project requires Vulkan 1.3 and uses modern features exclusively.

## Device Setup

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_queue(vk::QueueFlagBits::eCompute)
    .with_presentation(surface)              // For swapchain support
    .with_synchronization_2()                // Required: modern barriers
    .with_dynamic_rendering()                // Required: no render passes
    .with_descriptor_indexing()              // Optional: bindless resources
    .with_ray_tracing()                      // Optional: RT features
    .build();
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Vulkan/DeviceFinder.h`

---

## Required Features

### Synchronization2

Enables `VK_KHR_synchronization2` for modern barrier API.

**What it provides:**
- `vk::PipelineStageFlagBits2` and `vk::AccessFlagBits2` (64-bit flags)
- `cmd.pipelineBarrier2()` with `vk::DependencyInfo`
- More granular pipeline stages (`eColorAttachmentOutput`, `eRayTracingShaderKHR`)
- Better validation and debugging

**Usage in this project:**
```cpp
// All ResourceTracker operations use Synchronization2
tracker.request(vw::Barrier::ImageState{
    .image = image,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // 2!
    .access = vk::AccessFlagBits2::eColorAttachmentWrite          // 2!
});
```

### Dynamic Rendering

Enables `VK_KHR_dynamic_rendering` to avoid `VkRenderPass` and `VkFramebuffer`.

**What it provides:**
- `cmd.beginRendering()` / `cmd.endRendering()`
- Inline attachment specification via `vk::RenderingAttachmentInfo`
- Simpler API, no up-front render pass creation

**Usage in this project:**
```cpp
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = view,
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = clearColor
};

cmd.beginRendering(vk::RenderingInfo{
    .renderArea = {.offset = {0, 0}, .extent = extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachment,
    .pDepthAttachment = &depthAttachment  // Optional
});
// Draw commands...
cmd.endRendering();
```

---

## Optional Features

### Descriptor Indexing

Enables `VK_EXT_descriptor_indexing` for bindless resources.

**What it provides:**
- Non-uniform indexing into descriptor arrays
- Update-after-bind descriptors
- Partially bound descriptor sets
- Large descriptor counts (thousands of textures)

**When to use:**
- Material systems with many textures
- Dynamic object rendering
- GPU-driven rendering

```cpp
auto device = instance->findGpu()
    .with_descriptor_indexing()
    .build();
```

### Ray Tracing

Enables the full ray tracing extension suite:
- `VK_KHR_acceleration_structure` - BLAS/TLAS
- `VK_KHR_ray_tracing_pipeline` - RT shaders
- `VK_KHR_ray_query` - Inline ray tracing in any shader
- `VK_KHR_deferred_host_operations` - Async AS builds

**What it provides:**
- Hardware-accelerated ray-triangle intersection
- Bottom-level and top-level acceleration structures
- Ray generation, miss, closest-hit, any-hit shaders
- Shader binding tables

**Required companion feature:**
```cpp
// Ray tracing requires buffer device address
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**See:** [ray-tracing.md](ray-tracing.md) for detailed usage.

---

## Feature Validation

### Checking Device Support

```cpp
auto finder = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering();

// Check if any device matches requirements
auto physicalDevice = finder.get();  // Returns std::optional
if (!physicalDevice) {
    throw std::runtime_error("No compatible GPU found");
}

// Or build directly (throws if no match)
auto device = finder.build();
```

### Querying Available Extensions

```cpp
// DeviceFinder internally queries extensions
// Devices without required extensions are filtered out
finder.with_ray_tracing();  // Removes devices without RT support
```

---

## Device Selection Criteria

The `DeviceFinder` filters devices based on:

1. **Queue support**: Must have queues matching requested flags
2. **Extension availability**: Must support all requested extensions
3. **Feature support**: Must support enabled features (via feature chain)

### Selection Order

When multiple devices match:
- First matching device is selected
- No automatic preference for discrete vs integrated GPUs
- Use explicit device selection if needed

### Fallback Strategy

```cpp
// Try with optional features, fall back if unavailable
auto finder = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering();

// Try ray tracing first
auto rtFinder = finder;  // Copy
rtFinder.with_ray_tracing();

auto device = rtFinder.get()
    ? rtFinder.build()
    : finder.build();  // Fall back without RT
```

---

## Extension Dependencies

| Feature | Extensions Enabled |
|---------|-------------------|
| `with_synchronization_2()` | `VK_KHR_synchronization2` |
| `with_dynamic_rendering()` | `VK_KHR_dynamic_rendering` |
| `with_descriptor_indexing()` | `VK_EXT_descriptor_indexing` |
| `with_ray_tracing()` | `VK_KHR_acceleration_structure`, `VK_KHR_ray_tracing_pipeline`, `VK_KHR_ray_query`, `VK_KHR_deferred_host_operations` |
| `with_presentation(surface)` | `VK_KHR_swapchain` |

---

## Anti-Patterns

```cpp
// DON'T: Old pipeline stage flags
vk::PipelineStageFlagBits::eColorAttachmentOutput  // Old 32-bit

// DO: Synchronization2 flags
vk::PipelineStageFlagBits2::eColorAttachmentOutput  // New 64-bit

// DON'T: VkRenderPass/VkFramebuffer
vk::RenderPassBeginInfo beginInfo{...};
cmd.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

// DO: Dynamic rendering
cmd.beginRendering(vk::RenderingInfo{...});

// DON'T: Manual barrier construction
vk::ImageMemoryBarrier barrier{...};
cmd.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);

// DO: ResourceTracker with Synchronization2
tracker.request(state);
tracker.flush(cmd);  // Uses pipelineBarrier2
```

---

## Feature Chain Structure

The `DeviceFinder` manages a feature chain internally:

```cpp
vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceSynchronization2Features,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
    vk::PhysicalDeviceRayQueryFeaturesKHR,
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
    vk::PhysicalDeviceDynamicRenderingFeatures
> m_features;
```

This chain is passed to `vkCreateDevice` to enable all requested features.
