---
sidebar_position: 2
title: "Pipeline Layout"
---

# Pipeline Layout

## Overview

A **pipeline layout** defines the interface between shader code and the
application: which descriptor sets the shaders expect and what push constant
ranges are available.  Every graphics (or compute, or ray tracing) pipeline
requires a layout.

VulkanWrapper provides `PipelineLayout` and `PipelineLayoutBuilder` as RAII
wrappers.

### Where it lives in the library

| Item | Header |
|------|--------|
| `PipelineLayout` | `VulkanWrapper/Pipeline/PipelineLayout.h` |
| `PipelineLayoutBuilder` | `VulkanWrapper/Pipeline/PipelineLayout.h` |

Both live in the `vw` namespace.

---

## API Reference

### PipelineLayoutBuilder

| Method | Returns | Description |
|--------|---------|-------------|
| `PipelineLayoutBuilder(std::shared_ptr<const Device> device)` | -- | Constructs the builder. |
| `with_descriptor_set_layout(std::shared_ptr<const DescriptorSetLayout> layout)` | `PipelineLayoutBuilder&` | Add a descriptor set layout. Call once per set index (set 0, set 1, ...). The order of calls determines the set number. |
| `with_push_constant_range(vk::PushConstantRange range)` | `PipelineLayoutBuilder&` | Add a push constant range. |
| `build()` | `PipelineLayout` | Create the Vulkan pipeline layout. |

### PipelineLayout

`PipelineLayout` inherits `ObjectWithUniqueHandle<vk::UniquePipelineLayout>`.
It owns the native Vulkan handle and exposes it via `handle()`.

| Method | Returns | Description |
|--------|---------|-------------|
| `handle()` | `vk::PipelineLayout` | The raw Vulkan handle. Used when binding descriptor sets or pushing constants. |

---

## Usage Examples

### Empty layout (no descriptors, no push constants)

```cpp
auto layout = vw::PipelineLayoutBuilder(device).build();
```

This is useful for trivial shaders that do not read any external resources
(e.g., a fullscreen pass that only uses `gl_VertexIndex`).

### Layout with one descriptor set

```cpp
// Build a descriptor set layout first
auto setLayout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();

// Then reference it in the pipeline layout
auto layout =
    vw::PipelineLayoutBuilder(device)
        .with_descriptor_set_layout(setLayout)
        .build();
```

### Layout with multiple descriptor sets

```cpp
// Set 0: per-frame data (camera, lights)
auto frameLayout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
        .build();

// Set 1: per-material data (textures)
auto materialLayout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();

auto layout =
    vw::PipelineLayoutBuilder(device)
        .with_descriptor_set_layout(frameLayout)      // set 0
        .with_descriptor_set_layout(materialLayout)    // set 1
        .build();
```

### Layout with push constants

```cpp
struct PushConstants {
    glm::mat4 model;
    glm::vec4 color;
};

auto layout =
    vw::PipelineLayoutBuilder(device)
        .with_descriptor_set_layout(setLayout)
        .with_push_constant_range(
            vk::PushConstantRange(
                vk::ShaderStageFlagBits::eVertex |
                    vk::ShaderStageFlagBits::eFragment,
                0,                       // offset
                sizeof(PushConstants)))  // size
        .build();
```

### Using the layout at draw time

```cpp
// Push constants
PushConstants pc{modelMatrix, color};
cmd.pushConstants(
    pipeline->layout().handle(),
    vk::ShaderStageFlagBits::eVertex |
        vk::ShaderStageFlagBits::eFragment,
    0,
    sizeof(PushConstants),
    &pc);

// Bind descriptor sets
auto descriptorHandle = descriptorSet.handle();
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline->layout().handle(),
    0,  // firstSet
    1,  // descriptorSetCount
    &descriptorHandle,
    0, nullptr);
```

---

## Design Rationale

### Separation from Pipeline

The pipeline layout is a separate object because multiple pipelines can share
the same layout.  For example, a depth pre-pass and a lighting pass may use
different shaders but the same descriptor set layout and push constant ranges.
Sharing a layout means the driver does not need to rebind descriptors when
switching between those pipelines.

### Push constants vs uniform buffers

Push constants are a small (typically 128-256 bytes, GPU-dependent) block of
data that you can update directly in the command stream -- no buffer allocation,
no descriptor set update.  They are ideal for per-draw data like a model
matrix or a color value.

Use push constants when:
- The data is small (under 128 bytes is safe on all hardware).
- The data changes every draw call.
- You want zero allocation overhead.

Use uniform buffers when:
- The data is large.
- The data is shared across many draw calls.
- You need array indexing in the shader.

### Push constant GLSL declaration

```glsl
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 color;
} pc;

void main() {
    gl_Position = pc.model * vec4(inPosition, 1.0);
}
```

---

## Common Pitfalls

1. **Set index ordering matters.**
   `with_descriptor_set_layout()` calls are positional.  The first call defines
   set 0, the second defines set 1, and so on.  If your shader declares
   `layout(set = 1, binding = 0)` but you only added one descriptor set layout,
   the binding will be wrong.

2. **Push constant range overlap.**
   Vulkan allows multiple push constant ranges (one per shader stage), but they
   must not overlap for the same stage.  If your vertex and fragment shaders
   both need push constants, define non-overlapping ranges or a single range
   that covers both stages:

   ```cpp
   // Single range visible to both stages
   vk::PushConstantRange(
       vk::ShaderStageFlagBits::eVertex |
           vk::ShaderStageFlagBits::eFragment,
       0, sizeof(PushConstants))
   ```

3. **Layout mismatch between pipeline and descriptors.**
   The descriptor set layouts stored in the pipeline layout must match the
   layouts used when allocating descriptor sets.  If they differ, you will get
   validation errors or undefined behavior.

4. **Exceeding the push constant size limit.**
   The Vulkan spec guarantees at least 128 bytes.  If you need more, check
   `VkPhysicalDeviceLimits::maxPushConstantsSize`.  Exceeding the limit is
   a validation error.

5. **Using the layout after the Pipeline is destroyed.**
   The `Pipeline` object stores a copy of the `PipelineLayout`.  You can safely
   access `pipeline->layout()` for the lifetime of the pipeline.  However, if
   you store a separate `PipelineLayout` variable, make sure it outlives any
   command buffers that reference it.
