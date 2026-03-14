# Descriptors Module

`vw` namespace · `Descriptors/` directory · Tier 4

Descriptor set management for binding resources (buffers, images, acceleration structures) to shaders.

## DescriptorSetLayout / DescriptorSetLayoutBuilder

```cpp
auto layout = DescriptorSetLayoutBuilder(device)
    .add_binding(0, eUniformBuffer, eVertex | eFragment)
    .add_binding(1, eCombinedImageSampler, eFragment)
    .build();
```

## DescriptorPool

Allocates descriptor sets from a pool. Created with max set count and pool sizes.

## DescriptorSet

Bound resource set. Wraps `vk::DescriptorSet` with write operations for buffers, images, and acceleration structures.

## DescriptorAllocator

Higher-level allocator that manages pool creation and growth automatically.

## Vertex Concept (`Vertex.h`)

C++23 concept for vertex types. Requires two static methods:

```cpp
concept Vertex = requires {
    V::binding_description(binding);
    V::attribute_descriptions(binding, location);
};
```

Used by `GraphicsPipelineBuilder::add_vertex_binding<V>()` to auto-configure vertex input state. Vertex types include `Vertex3D` (position + normal) and `FullVertex3D` (position + normal + tangent + bitangent + UV).
