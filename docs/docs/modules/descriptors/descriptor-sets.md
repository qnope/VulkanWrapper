---
sidebar_position: 1
title: "Descriptor Sets"
---

# Descriptor Sets

## Overview

In Vulkan, **descriptors** are the mechanism for making GPU resources (buffers,
images, samplers, acceleration structures) visible to shaders.  They are
organized into **descriptor sets**, and each set follows a **descriptor set
layout** that describes what kinds of resources it contains.

VulkanWrapper provides:

- **`DescriptorSetLayoutBuilder`** -- fluent builder for creating layouts with
  automatic binding index numbering.
- **`DescriptorSetLayout`** -- RAII wrapper for a layout handle.
- **`DescriptorPool`** / **`DescriptorPoolBuilder`** -- manages allocation of
  descriptor sets with automatic pool growth.
- **`DescriptorSet`** -- lightweight handle wrapping `vk::DescriptorSet`, plus
  tracked resource state for automatic barrier generation.
- **`Vertex` concept** and built-in vertex types -- define vertex input formats
  for the pipeline.

### Where it lives in the library

| Item | Header |
|------|--------|
| `DescriptorSetLayout`, `DescriptorSetLayoutBuilder` | `VulkanWrapper/Descriptors/DescriptorSetLayout.h` |
| `DescriptorPool`, `DescriptorPoolBuilder` | `VulkanWrapper/Descriptors/DescriptorPool.h` |
| `DescriptorSet` | `VulkanWrapper/Descriptors/DescriptorSet.h` |
| `Vertex` concept, vertex types | `VulkanWrapper/Descriptors/Vertex.h` |

All types live in the `vw` namespace.

---

## API Reference

### DescriptorSetLayoutBuilder

The builder uses **implicit binding numbering**.  Each `with_*` call
automatically assigns the next binding index (starting at 0).

| Method | Returns | Description |
|--------|---------|-------------|
| `DescriptorSetLayoutBuilder(std::shared_ptr<const Device> device)` | -- | Construct the builder. |
| `with_uniform_buffer(vk::ShaderStageFlags stages, int number)` | `DescriptorSetLayoutBuilder&` | Add `number` uniform buffer binding(s) visible to `stages`. |
| `with_storage_buffer(vk::ShaderStageFlags stages, int number = 1)` | `DescriptorSetLayoutBuilder&` | Add storage buffer binding(s). |
| `with_combined_image(vk::ShaderStageFlags stages, int number)` | `DescriptorSetLayoutBuilder&` | Add combined image sampler binding(s). |
| `with_sampled_image(vk::ShaderStageFlags stages, int number)` | `DescriptorSetLayoutBuilder&` | Add sampled image binding(s) (image without sampler). |
| `with_storage_image(vk::ShaderStageFlags stages, int number)` | `DescriptorSetLayoutBuilder&` | Add storage image binding(s) (for compute writes). |
| `with_input_attachment(vk::ShaderStageFlags stages)` | `DescriptorSetLayoutBuilder&` | Add an input attachment binding. |
| `with_sampler(vk::ShaderStageFlags stages)` | `DescriptorSetLayoutBuilder&` | Add a standalone sampler binding. |
| `with_acceleration_structure(vk::ShaderStageFlags stages)` | `DescriptorSetLayoutBuilder&` | Add a ray tracing acceleration structure binding. |
| `with_sampled_images_bindless(vk::ShaderStageFlags stages, uint32_t max_count)` | `DescriptorSetLayoutBuilder&` | Add a variable-count bindless sampled image array. |
| `with_storage_buffer_bindless(vk::ShaderStageFlags stages)` | `DescriptorSetLayoutBuilder&` | Add a variable-count bindless storage buffer. |
| `build()` | `std::shared_ptr<DescriptorSetLayout>` | Create the layout. Returns a shared pointer because layouts are frequently shared across pools and pipelines. |

### DescriptorSetLayout

| Method | Returns | Description |
|--------|---------|-------------|
| `handle()` | `vk::DescriptorSetLayout` | The raw Vulkan handle. |
| `get_pool_sizes()` | `std::vector<vk::DescriptorPoolSize>` | Returns pool size requirements for allocating sets with this layout. Used internally by `DescriptorPool`. |

### DescriptorPoolBuilder

| Method | Returns | Description |
|--------|---------|-------------|
| `DescriptorPoolBuilder(std::shared_ptr<const Device> device, const std::shared_ptr<const DescriptorSetLayout>& layout)` | -- | Construct the builder for a specific layout. |
| `with_update_after_bind()` | `DescriptorPoolBuilder&` | Enable the `VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT` flag. Required for bindless descriptors. |
| `build()` | `DescriptorPool` | Create the pool. |

### DescriptorPool

| Method | Returns | Description |
|--------|---------|-------------|
| `layout()` | `std::shared_ptr<const DescriptorSetLayout>` | The layout this pool allocates from. |
| `allocate_set()` | `DescriptorSet` | Allocate a descriptor set (uninitialized -- you must write to it before use). |
| `allocate_set(const DescriptorAllocator& allocator)` | `DescriptorSet` | Allocate a descriptor set and immediately write descriptors from the allocator. If a set with the same allocator state already exists, returns the cached set. |
| `update_set(vk::DescriptorSet set, const DescriptorAllocator& allocator)` | `void` | Update an existing set with new descriptor writes. |

The pool grows automatically -- if the internal Vulkan pool runs out of space,
a new one is created transparently.

### DescriptorSet

| Method | Returns | Description |
|--------|---------|-------------|
| `handle()` | `vk::DescriptorSet` | The raw Vulkan handle. |
| `resources()` | `const std::vector<Barrier::ResourceState>&` | The resource states tracked by this set. Used to automatically issue correct barriers before rendering. |

---

## Vertex Types and the Vertex Concept

### The Vertex concept

```cpp
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> &&
    std::is_trivially_copyable_v<T> &&
    requires(T x) {
        { T::binding_description(0) }
            -> std::same_as<vk::VertexInputBindingDescription>;
        { T::attribute_descriptions(0, 0)[0] }
            -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };
```

Any type satisfying this concept can be used with
`GraphicsPipelineBuilder::add_vertex_binding<V>()`.

### VertexInterface base class

`VertexInterface<Ts...>` is a helper that automatically generates
`binding_description()` and `attribute_descriptions()` from a parameter pack of
field types.  Inherit from it to create custom vertex types:

```cpp
struct MyVertex : vw::VertexInterface<glm::vec3, glm::vec2> {
    glm::vec3 position;
    glm::vec2 uv;
};
```

Format mapping (`format_from<T>`):

| C++ Type | Vulkan Format |
|----------|---------------|
| `float` | `eR32Sfloat` |
| `glm::vec2` | `eR32G32Sfloat` |
| `glm::vec3` | `eR32G32B32Sfloat` |
| `glm::vec4` | `eR32G32B32A32Sfloat` |

### Built-in vertex types

| Type | Fields |
|------|--------|
| `Vertex3D` | `glm::vec3 position` |
| `ColoredVertex3D` | `glm::vec3 position`, `glm::vec3 color` |
| `ColoredVertex2D` | `glm::vec2 position`, `glm::vec3 color` |
| `ColoredAndTexturedVertex3D` | `glm::vec3 position`, `glm::vec3 color`, `glm::vec2 texCoord` |
| `ColoredAndTexturedVertex2D` | `glm::vec2 position`, `glm::vec3 color`, `glm::vec2 texCoord` |
| `FullVertex3D` | `glm::vec3 position`, `glm::vec3 normal`, `glm::vec3 tangeant`, `glm::vec3 bitangeant`, `glm::vec2 uv` |

---

## Usage Examples

### Basic layout + pool + set

```cpp
// 1. Define what the shader expects
auto layout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)      // binding 0
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)    // binding 1
        .build();

// 2. Create a pool for this layout
auto pool = vw::DescriptorPoolBuilder(device, layout).build();

// 3. Populate a DescriptorAllocator with the actual resources
vw::DescriptorAllocator allocator;
allocator.add_uniform_buffer(
    0,  // binding
    uniformBuffer.handle(), 0, sizeof(UBO),
    vk::PipelineStageFlagBits2::eVertexShader,
    vk::AccessFlagBits2::eUniformRead);

allocator.add_combined_image(
    1,  // binding
    vw::CombinedImage(textureView, sampler),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead);

// 4. Allocate a set and write descriptors in one step
auto set = pool.allocate_set(allocator);
```

### Using descriptor resources for barrier tracking

```cpp
// The DescriptorSet stores the resource states from the allocator.
// Request barriers for all descriptor resources before rendering:
for (const auto& resource : set.resources()) {
    tracker.request(resource);
}
tracker.flush(cmd);
```

### Bindless textures

```cpp
auto layout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_sampler(vk::ShaderStageFlagBits::eFragment)              // binding 0
        .with_sampled_images_bindless(
            vk::ShaderStageFlagBits::eFragment, 1024)                  // binding 1
        .build();

auto pool =
    vw::DescriptorPoolBuilder(device, layout)
        .with_update_after_bind()
        .build();
```

### Empty descriptor set (for pipelines that require a set but have no bindings)

```cpp
auto emptyLayout = vw::DescriptorSetLayoutBuilder(device).build();
auto emptyPool   = vw::DescriptorPoolBuilder(device, emptyLayout).build();

vw::DescriptorAllocator allocator;
auto emptySet = emptyPool.allocate_set(allocator);
```

### Custom vertex type

```cpp
struct ParticleVertex : vw::VertexInterface<glm::vec3, glm::vec4, float> {
    glm::vec3 position;   // location 0: vec3
    glm::vec4 color;      // location 1: vec4
    float     size;        // location 2: float
};

// Use in pipeline builder:
auto pipeline =
    vw::GraphicsPipelineBuilder(device, layout)
        .add_vertex_binding<ParticleVertex>()
        // ...
        .build();
```

### Corresponding GLSL declarations

```glsl
// Must match the DescriptorSetLayoutBuilder call order
layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 0, binding = 1) uniform sampler2D albedoTex;
```

---

## Design Rationale

### Automatic binding numbering

The `DescriptorSetLayoutBuilder` assigns binding indices implicitly (0, 1,
2, ...).  This trades flexibility for safety -- you cannot accidentally leave
gaps or duplicate a binding index.  The order of `with_*` calls must match the
`binding = N` declarations in your shader.

### Resource tracking in DescriptorSet

Each `DescriptorSet` stores a snapshot of the `Barrier::ResourceState` for every
resource it references.  When you iterate `set.resources()` and pass each state
to a `ResourceTracker`, the tracker knows what layout transitions and access
barriers are needed before rendering.  This eliminates a common class of
"forgot to transition the image" bugs.

### Pool auto-growth

The `DescriptorPool` creates Vulkan descriptor pools internally and
automatically allocates new ones when the current pool is full.  You never need
to predict pool sizes upfront.

### Descriptor set caching

When you call `pool.allocate_set(allocator)`, the pool checks whether a set
with identical descriptor writes already exists.  If so, it returns the cached
set.  This is an important optimization for render loops where the same
resources are bound every frame.

---

## Common Pitfalls

1. **Binding order mismatch between builder and shader.**
   If your shader declares `layout(set = 0, binding = 0) uniform sampler2D tex`
   but the builder's first call is `with_uniform_buffer(...)`, binding 0 will
   be a uniform buffer, not a combined image sampler.  Always match the builder
   call order to the shader binding order.

2. **Forgetting `with_update_after_bind()` for bindless.**
   Bindless descriptor arrays require the `UPDATE_AFTER_BIND` flag on both the
   layout (handled automatically by `with_sampled_images_bindless`) and the pool
   (you must call `with_update_after_bind()` on `DescriptorPoolBuilder`).

3. **Writing to a set after it has been bound.**
   Standard (non-bindless) descriptor sets must not be updated while they are
   in use by the GPU.  Either double-buffer your sets or wait for the fence
   before updating.

4. **Mixing descriptor types.**
   If the layout says binding 0 is a `uniform buffer` but the allocator writes
   a `combined image` to binding 0, you will get a validation error.  The
   `DescriptorAllocator` does not cross-check against the layout at write time.

5. **Vertex attribute format mismatch.**
   If you define a custom vertex struct and the fields do not match what the
   shader's `layout(location = N) in` expects, you will get garbage data or
   validation errors.  Make sure the C++ field types and the GLSL `in` types
   agree (e.g., `glm::vec3` maps to `vec3`).

6. **`number` parameter confusion.**
   The `number` parameter in `with_uniform_buffer(stages, number)` refers to
   the descriptor count for that binding (e.g., an array of 4 uniform buffers).
   For a single resource, pass `1`.  This is different from the binding index,
   which is assigned automatically.
