---
sidebar_position: 2
---

# Pipeline Layout

The `PipelineLayout` class defines the interface between shaders and resources (descriptors and push constants).

## Overview

```cpp
#include <VulkanWrapper/Pipeline/PipelineLayout.h>

auto layout = PipelineLayoutBuilder()
    .setDevice(device)
    .addDescriptorSetLayout(materialLayout)
    .addDescriptorSetLayout(sceneLayout)
    .addPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex)
    .build();
```

## PipelineLayoutBuilder

### Methods

| Method | Description |
|--------|-------------|
| `setDevice(shared_ptr<Device>)` | Set the logical device |
| `addDescriptorSetLayout(layout)` | Add a descriptor set layout |
| `addPushConstantRange<T>(stages)` | Add push constant range |

### Adding Descriptor Sets

```cpp
// Add layouts in set order (set 0, set 1, ...)
builder.addDescriptorSetLayout(set0Layout)   // Set 0
       .addDescriptorSetLayout(set1Layout);  // Set 1
```

### Push Constants

```cpp
// Define push constant structure
struct PushConstants {
    glm::mat4 modelMatrix;
    glm::vec4 color;
};

// Add push constant range
builder.addPushConstantRange<PushConstants>(
    vk::ShaderStageFlagBits::eVertex |
    vk::ShaderStageFlagBits::eFragment
);
```

## PipelineLayout Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::PipelineLayout` | Get raw handle |
| `descriptor_set_layouts()` | `span<const shared_ptr<...>>` | Get set layouts |

### Usage

```cpp
// Bind descriptor sets
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    layout.handle(),
    0,               // First set
    descriptorSets,  // Sets to bind
    dynamicOffsets   // Dynamic offsets
);

// Push constants
cmd.pushConstants(
    layout.handle(),
    vk::ShaderStageFlagBits::eVertex,
    0,               // Offset
    sizeof(pushData),
    &pushData
);
```

## Push Constants

Push constants are small, fast-path data updates:

### Size Limits

- Guaranteed minimum: 128 bytes
- Check device limits for actual maximum

### Best Practices

```cpp
// Keep push constants small and aligned
struct PushConstants {
    glm::mat4 transform;   // 64 bytes
    glm::vec4 params;      // 16 bytes
    // Total: 80 bytes
};
```

### In Shaders

GLSL:
```glsl
layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec4 params;
} pc;

void main() {
    gl_Position = pc.transform * position;
}
```

## Multiple Push Constant Ranges

Different stages can have different ranges:

```cpp
builder.addPushConstantRange<VertexPushConstants>(
    vk::ShaderStageFlagBits::eVertex,
    0  // Offset
);

builder.addPushConstantRange<FragmentPushConstants>(
    vk::ShaderStageFlagBits::eFragment,
    sizeof(VertexPushConstants)  // Offset after vertex data
);
```

## Complete Example

```cpp
// Create descriptor set layouts
auto materialLayout = DescriptorSetLayoutBuilder()
    .setDevice(device)
    .addBinding(0, vk::DescriptorType::eCombinedImageSampler,
                vk::ShaderStageFlagBits::eFragment)
    .build();

auto sceneLayout = DescriptorSetLayoutBuilder()
    .setDevice(device)
    .addBinding(0, vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eVertex)
    .addBinding(1, vk::DescriptorType::eStorageBuffer,
                vk::ShaderStageFlagBits::eVertex)
    .build();

// Push constants
struct PushConstants {
    glm::mat4 model;
    uint32_t materialId;
    float padding[3];
};

// Create pipeline layout
auto layout = PipelineLayoutBuilder()
    .setDevice(device)
    .addDescriptorSetLayout(materialLayout)  // Set 0
    .addDescriptorSetLayout(sceneLayout)     // Set 1
    .addPushConstantRange<PushConstants>(
        vk::ShaderStageFlagBits::eVertex |
        vk::ShaderStageFlagBits::eFragment)
    .build();
```

## Best Practices

1. **Order set layouts logically** - frequently updated sets first
2. **Keep push constants small** - under 128 bytes
3. **Align push constant data** - use padding if needed
4. **Share layouts** between compatible pipelines
5. **Consider bindless** for many textures
