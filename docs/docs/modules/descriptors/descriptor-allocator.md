---
sidebar_position: 2
---

# Descriptor Allocator

The `DescriptorAllocator` provides high-level descriptor set management with automatic pool handling and update tracking.

## Overview

```cpp
#include <VulkanWrapper/Descriptors/DescriptorAllocator.h>

DescriptorAllocator allocator(device);

// Allocate and update in one step
auto set = allocator.allocate(layout, {
    {0, uniformBufferInfo},
    {1, imageInfo}
});
```

## DescriptorAllocator

### Constructor

```cpp
DescriptorAllocator(std::shared_ptr<const Device> device);
```

### Allocating Sets

```cpp
// Allocate with immediate updates
auto set = allocator.allocate(layout, {
    {0, bufferInfo},    // Binding 0
    {1, imageInfo},     // Binding 1
    {2, storageInfo}    // Binding 2
});
```

### Deferred Updates

```cpp
// Allocate without updates
auto set = allocator.allocate(layout);

// Update later
allocator.update(set, 0, bufferInfo);
allocator.update(set, 1, imageInfo);
```

## Binding Info Types

### Buffer Binding

```cpp
vk::DescriptorBufferInfo bufferInfo{
    .buffer = buffer->handle(),
    .offset = 0,
    .range = sizeof(Data)  // or VK_WHOLE_SIZE
};
```

### Image Binding

```cpp
vk::DescriptorImageInfo imageInfo{
    .sampler = sampler->handle(),
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};
```

### Acceleration Structure Binding

```cpp
vk::WriteDescriptorSetAccelerationStructureKHR asInfo{
    .accelerationStructureCount = 1,
    .pAccelerationStructures = &tlas->handle()
};
```

## Pool Management

The allocator automatically manages descriptor pools:

- Creates pools as needed
- Tracks pool usage
- Allocates from available pools

```cpp
// Internal pool creation
auto pool = DescriptorPoolBuilder()
    .setDevice(device)
    .setMaxSets(1000)  // Default pool size
    .addPoolSize(vk::DescriptorType::eUniformBuffer, 1000)
    .addPoolSize(vk::DescriptorType::eCombinedImageSampler, 2000)
    // ... other types
    .build();
```

## Update Batching

Batch multiple updates for efficiency:

```cpp
// Begin batch
allocator.begin_batch();

// Queue updates
for (const auto& object : objects) {
    allocator.update(set, 0, object.bufferInfo);
}

// Flush all updates
allocator.end_batch();
```

## Common Patterns

### Per-Frame Descriptors

```cpp
// Allocate per-frame sets
std::array<DescriptorSet, FRAMES_IN_FLIGHT> frameSets;
for (auto& set : frameSets) {
    set = allocator.allocate(frameLayout);
}

// Update each frame
void update(uint32_t frameIndex) {
    allocator.update(frameSets[frameIndex], 0, cameraBuffer);
}
```

### Material Descriptors

```cpp
// Allocate material set
auto materialSet = allocator.allocate(materialLayout, {
    {0, albedoTexture},
    {1, normalTexture},
    {2, metallicRoughnessTexture}
});

// Store with material
material.descriptorSet = materialSet;
```

### Scene Descriptors

```cpp
// Create scene descriptor set
auto sceneSet = allocator.allocate(sceneLayout, {
    {0, cameraBuffer},
    {1, lightsBuffer},
    {2, accelerationStructure}
});
```

## Reset and Cleanup

```cpp
// Reset all pools (frees all allocated sets)
allocator.reset();

// Or let destructor clean up
```

## Best Practices

1. **Use one allocator** per logical grouping
2. **Batch updates** when updating many sets
3. **Preallocate** for known set counts
4. **Reset periodically** to reclaim memory
5. **Group by update frequency** for efficient updates
