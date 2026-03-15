---
sidebar_position: 2
title: "Descriptor Allocator"
---

# Descriptor Allocator

## Overview

The **`DescriptorAllocator`** is a helper class that accumulates descriptor
write operations for a single descriptor set.  It collects all the resource
bindings (buffers, images, samplers, acceleration structures) together with
their synchronization metadata (pipeline stage and access flags), and then
produces either:

- A `std::vector<vk::WriteDescriptorSet>` for Vulkan descriptor updates, or
- A `std::vector<Barrier::ResourceState>` for automatic barrier generation.

The typical flow is:

1. Create a `DescriptorAllocator` (default-constructed, no arguments).
2. Call `add_*` methods to register each resource with its binding index and
   synchronization info.
3. Pass the allocator to `DescriptorPool::allocate_set(allocator)`, which
   allocates a set and writes all descriptors in one shot.

### Where it lives in the library

| Item | Header |
|------|--------|
| `DescriptorAllocator` | `VulkanWrapper/Descriptors/DescriptorAllocator.h` |

The class lives in the `vw` namespace.

---

## API Reference

### Construction

```cpp
vw::DescriptorAllocator allocator;  // default constructor, no arguments
```

`DescriptorAllocator` is value-semantic and supports equality comparison
(`operator==`). The equality check is used by `DescriptorPool` to cache and
reuse descriptor sets.

### Adding resources

Every `add_*` method takes:
- A **binding** index (must match the descriptor set layout).
- The **resource handle** (buffer, image view, sampler, etc.).
- **Pipeline stage** (`vk::PipelineStageFlags2`) and **access flags**
  (`vk::AccessFlags2`) using Synchronization2 types that describe how the
  shader will use the resource.
- An optional **array_element** index (defaults to 0) for descriptor arrays.

| Method | Description |
|--------|-------------|
| `add_uniform_buffer(int binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a uniform buffer region. |
| `add_storage_buffer(int binding, vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a storage buffer region. |
| `add_combined_image(int binding, const CombinedImage& image, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a combined image sampler. `CombinedImage` bundles an `ImageView` and a `Sampler`. |
| `add_sampled_image(int binding, const ImageView& image_view, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a sampled image (no sampler). Also has a `shared_ptr<const ImageView>` overload. |
| `add_storage_image(int binding, const ImageView& image_view, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a storage image (compute/ray tracing shader write). |
| `add_input_attachment(int binding, std::shared_ptr<const ImageView> image_view, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register an input attachment. |
| `add_sampler(int binding, vk::Sampler sampler, uint32_t array_element = 0)` | Register a standalone sampler (no stage/access needed). |
| `add_acceleration_structure(int binding, vk::AccelerationStructureKHR tlas, vk::PipelineStageFlags2 stage, vk::AccessFlags2 access, uint32_t array_element = 0)` | Register a ray tracing acceleration structure. |

### Querying

| Method | Returns | Description |
|--------|---------|-------------|
| `get_write_descriptors()` | `std::vector<vk::WriteDescriptorSet>` | The Vulkan write descriptor operations. Used internally by `DescriptorPool`. |
| `get_resources()` | `std::vector<Barrier::ResourceState>` | The resource states for barrier tracking. Each entry is a `std::variant<ImageState, BufferState, AccelerationStructureState>`. |
| `operator==(const DescriptorAllocator&)` | `bool` | Equality comparison (defaulted). Used by `DescriptorPool` to cache and reuse sets. |

---

## Usage Examples

### Uniform buffer + texture

```cpp
vw::DescriptorAllocator allocator;

// Binding 0: uniform buffer (vertex shader)
allocator.add_uniform_buffer(
    0,
    ubo.handle(), 0, sizeof(UniformData),
    vk::PipelineStageFlagBits2::eVertexShader,
    vk::AccessFlagBits2::eUniformRead);

// Binding 1: combined image sampler (fragment shader)
allocator.add_combined_image(
    1,
    vw::CombinedImage(textureView, sampler),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead);

// Allocate and write in one step
auto set = pool.allocate_set(allocator);
```

### Storage buffers for compute

```cpp
vw::DescriptorAllocator allocator;

allocator.add_storage_buffer(
    0,
    inputBuffer.handle(), 0, inputSize,
    vk::PipelineStageFlagBits2::eComputeShader,
    vk::AccessFlagBits2::eShaderRead);

allocator.add_storage_buffer(
    1,
    outputBuffer.handle(), 0, outputSize,
    vk::PipelineStageFlagBits2::eComputeShader,
    vk::AccessFlagBits2::eShaderWrite);

auto set = pool.allocate_set(allocator);
```

### Ray tracing acceleration structure

```cpp
vw::DescriptorAllocator allocator;

allocator.add_acceleration_structure(
    0,
    tlas.handle(),
    vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    vk::AccessFlagBits2::eAccelerationStructureReadKHR);

allocator.add_storage_image(
    1,
    outputImageView,
    vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    vk::AccessFlagBits2::eShaderWrite);

auto set = pool.allocate_set(allocator);
```

### Separate sampler + sampled image

```cpp
vw::DescriptorAllocator allocator;

// Binding 0: standalone sampler
allocator.add_sampler(0, sampler->handle());

// Binding 1: sampled image (without sampler)
allocator.add_sampled_image(
    1,
    *textureView,
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead);

auto set = pool.allocate_set(allocator);
```

### Using resource states for barriers

The key integration point with the synchronization system:

```cpp
auto set = pool.allocate_set(allocator);

// Before rendering, tell the ResourceTracker what states are needed
for (const auto& resource : set.resources()) {
    tracker.request(resource);
}
tracker.flush(cmd);

// Now safe to render with this descriptor set
auto descriptorHandle = set.handle();
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline->layout().handle(),
    0, 1, &descriptorHandle, 0, nullptr);
```

### Descriptor array elements

For descriptor arrays (e.g., an array of textures), use the `array_element`
parameter:

```cpp
vw::DescriptorAllocator allocator;

for (uint32_t i = 0; i < textures.size(); ++i) {
    allocator.add_sampled_image(
        0,  // binding
        *textures[i],
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead,
        i); // array_element
}
```

---

## Design Rationale

### Why stage and access flags in the allocator?

Traditional Vulkan code separates descriptor writes from barrier management,
making it easy to forget a layout transition.  By requiring stage and access
flags at the point where you register a resource, the `DescriptorAllocator`
can produce `ResourceState` objects that the `ResourceTracker` uses to emit
correct barriers automatically.

This means: **if you register a resource in the allocator, the barrier
information is already captured**.  You just iterate `set.resources()` and
pass them to the tracker.

### Why the allocator is value-semantic

`DescriptorAllocator` supports `operator==`, which the `DescriptorPool` uses
as a cache key.  If you call `pool.allocate_set(allocator)` twice with the same
allocator state, the pool returns the previously allocated set without creating
a new one.  This is an important optimization for render loops where the
same resources are bound every frame.

### CombinedImage vs separate image + sampler

Vulkan supports two styles of image sampling:

- **Combined image sampler** (`VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`):
  image view and sampler are bundled into a single descriptor.  Use
  `add_combined_image()` with a `vw::CombinedImage(view, sampler)`.
- **Separate sampled image + sampler** (`VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE` +
  `VK_DESCRIPTOR_TYPE_SAMPLER`): more flexible, allows mixing and matching.
  Use `add_sampled_image()` + `add_sampler()` separately.

The combined approach is simpler and more common.  The separate approach is
needed for bindless texture arrays where you want one sampler shared across
many textures.

---

## Common Pitfalls

1. **Wrong binding index.**
   The `binding` parameter must match the descriptor set layout.  If the layout
   builder's first call was `with_uniform_buffer(...)` (binding 0) but you call
   `allocator.add_uniform_buffer(1, ...)`, binding 0 will be uninitialized and
   the GPU may read garbage.

2. **Mismatched descriptor type.**
   If the layout declares binding 0 as a `combined_image` but you call
   `add_uniform_buffer(0, ...)`, you will get a Vulkan validation error.

3. **Incorrect stage/access flags.**
   The stage and access flags must reflect how the shader actually uses the
   resource.  For example, if a texture is read in the fragment shader, use
   `vk::PipelineStageFlagBits2::eFragmentShader` and
   `vk::AccessFlagBits2::eShaderRead`.  Using `eVertexShader` would produce an
   incorrect barrier, potentially causing the GPU to read stale data.

4. **Forgetting to iterate `set.resources()` for barriers.**
   Allocating a descriptor set does not issue any barriers.  You must
   explicitly request the resource transitions via the `ResourceTracker` before
   the draw call, or the images may be in the wrong layout.

5. **Buffer offset and size mismatch.**
   When calling `add_uniform_buffer()` or `add_storage_buffer()`, the `offset`
   and `size` must describe the exact region the shader will read.  If `size` is
   larger than the actual buffer, you will get a validation error.

6. **Using v1 synchronization types.**
   Always use `vk::PipelineStageFlagBits2` and `vk::AccessFlags2` (the
   Synchronization2 versions).  The v1 types (`vk::PipelineStageFlagBits` and
   `vk::AccessFlagBits`) are a different type and will not compile with the
   `DescriptorAllocator` API.

7. **Constructing with arguments.**
   `DescriptorAllocator` takes no constructor arguments -- just default-
   construct it and call `add_*` methods.  The old documentation showed a
   device parameter; this is incorrect.
