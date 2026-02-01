---
sidebar_position: 1
---

# Descriptor Sets

Descriptor sets bind resources (buffers, images, samplers) to shader bindings.

## Overview

```cpp
#include <VulkanWrapper/Descriptors/DescriptorSetLayout.h>
#include <VulkanWrapper/Descriptors/DescriptorPool.h>
#include <VulkanWrapper/Descriptors/DescriptorSet.h>

// Create layout
auto layout = DescriptorSetLayoutBuilder()
    .setDevice(device)
    .addBinding(0, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eVertex)
    .addBinding(1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment)
    .build();

// Create pool
auto pool = DescriptorPoolBuilder()
    .setDevice(device)
    .setMaxSets(100)
    .addPoolSize(vk::DescriptorType::eUniformBuffer, 100)
    .addPoolSize(vk::DescriptorType::eCombinedImageSampler, 100)
    .build();

// Allocate set
auto set = pool->allocate(layout);
```

## DescriptorSetLayout

Defines the structure of a descriptor set.

### DescriptorSetLayoutBuilder

| Method | Description |
|--------|-------------|
| `setDevice(device)` | Set the logical device |
| `addBinding(binding, type, stages)` | Add a binding |
| `addBinding(binding, type, stages, count)` | Add array binding |

### Descriptor Types

| Type | Description |
|------|-------------|
| `eUniformBuffer` | Uniform buffer (read-only) |
| `eStorageBuffer` | Storage buffer (read-write) |
| `eCombinedImageSampler` | Image + sampler combined |
| `eSampledImage` | Image for sampling |
| `eStorageImage` | Image for read-write |
| `eUniformBufferDynamic` | UBO with dynamic offset |
| `eStorageBufferDynamic` | SSBO with dynamic offset |
| `eInputAttachment` | Subpass input |
| `eAccelerationStructureKHR` | Ray tracing AS |

### Example

```cpp
auto layout = DescriptorSetLayoutBuilder()
    .setDevice(device)
    // Binding 0: Camera UBO
    .addBinding(0, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eVertex)
    // Binding 1: Albedo texture
    .addBinding(1, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment)
    // Binding 2: Material buffer
    .addBinding(2, vk::DescriptorType::eStorageBuffer,
                vk::ShaderStageFlagBits::eFragment)
    // Binding 3: Texture array (8 textures)
    .addBinding(3, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment, 8)
    .build();
```

## DescriptorPool

Pool for allocating descriptor sets.

### DescriptorPoolBuilder

| Method | Description |
|--------|-------------|
| `setDevice(device)` | Set the logical device |
| `setMaxSets(count)` | Maximum sets to allocate |
| `addPoolSize(type, count)` | Add descriptor capacity |
| `setFlags(flags)` | Set pool flags |

### Example

```cpp
auto pool = DescriptorPoolBuilder()
    .setDevice(device)
    .setMaxSets(100)
    .addPoolSize(vk::DescriptorType::eUniformBuffer, 100)
    .addPoolSize(vk::DescriptorType::eCombinedImageSampler, 200)
    .addPoolSize(vk::DescriptorType::eStorageBuffer, 50)
    .build();
```

### Allocating Sets

```cpp
// Allocate single set
auto set = pool->allocate(layout);

// Allocate multiple sets
auto sets = pool->allocate(layout, 10);
```

## DescriptorSet

Represents an allocated descriptor set.

### Updating Descriptors

```cpp
// Update uniform buffer
vk::DescriptorBufferInfo bufferInfo{
    .buffer = uniformBuffer->handle(),
    .offset = 0,
    .range = sizeof(UniformData)
};

vk::WriteDescriptorSet write{
    .dstSet = set->handle(),
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = vk::DescriptorType::eUniformBuffer,
    .pBufferInfo = &bufferInfo
};

device->handle().updateDescriptorSets(write, {});
```

### Update Image

```cpp
vk::DescriptorImageInfo imageInfo{
    .sampler = sampler->handle(),
    .imageView = imageView->handle(),
    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
};

vk::WriteDescriptorSet write{
    .dstSet = set->handle(),
    .dstBinding = 1,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    .pImageInfo = &imageInfo
};

device->handle().updateDescriptorSets(write, {});
```

## Binding Descriptor Sets

```cpp
// In render loop
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipelineLayout.handle(),
    0,              // First set index
    set->handle(),  // Sets to bind
    {}              // Dynamic offsets
);
```

## Dynamic Offsets

For dynamic uniform/storage buffers:

```cpp
// Layout with dynamic UBO
auto layout = DescriptorSetLayoutBuilder()
    .addBinding(0, vk::DescriptorType::eUniformBufferDynamic,
                vk::ShaderStageFlagBits::eVertex)
    .build();

// Bind with offset
uint32_t offset = frameIndex * alignedSize;
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipelineLayout.handle(),
    0,
    set->handle(),
    {offset}  // Dynamic offset
);
```

## In Shaders

```glsl
// Uniform buffer
layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
} camera;

// Combined image sampler
layout(set = 0, binding = 1) uniform sampler2D albedoTex;

// Storage buffer
layout(set = 0, binding = 2) buffer Materials {
    MaterialData materials[];
};

// Array of samplers
layout(set = 0, binding = 3) uniform sampler2D textures[8];
```

## Best Practices

1. **Group by update frequency** - static vs per-frame sets
2. **Use dynamic offsets** for per-object data
3. **Pre-allocate pools** with enough capacity
4. **Reuse descriptor sets** when possible
5. **Consider bindless** for many textures
