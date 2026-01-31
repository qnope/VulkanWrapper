---
sidebar_position: 1
---

# Acceleration Structures

VulkanWrapper provides abstractions for Vulkan ray tracing acceleration structures.

## Overview

Ray tracing uses a two-level acceleration structure hierarchy:
- **BLAS** (Bottom-Level): Contains geometry (triangles)
- **TLAS** (Top-Level): Contains instances of BLAS with transforms

```cpp
#include <VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h>
#include <VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h>

// Build BLAS for each mesh
auto blas = BottomLevelAccelerationStructure::build(
    device, allocator, cmd,
    vertexBuffer, indexBuffer,
    vertexCount, indexCount
);

// Build TLAS with instances
auto tlas = TopLevelAccelerationStructure::build(
    device, allocator, cmd,
    instances
);
```

## Bottom-Level Acceleration Structure

Contains the actual geometry data.

### Building BLAS

```cpp
auto blas = BottomLevelAccelerationStructure::build(
    device,
    allocator,
    commandBuffer,
    vertexBuffer,          // Vertex positions
    indexBuffer,           // Triangle indices
    vertexCount,           // Number of vertices
    indexCount,            // Number of indices
    sizeof(Vertex),        // Vertex stride
    offsetof(Vertex, pos)  // Position offset in vertex
);
```

### BLAS with Custom Geometry

```cpp
vk::AccelerationStructureGeometryTrianglesDataKHR triangles{
    .vertexFormat = vk::Format::eR32G32B32Sfloat,
    .vertexData = vertexBuffer->device_address(),
    .vertexStride = sizeof(Vertex),
    .maxVertex = vertexCount - 1,
    .indexType = vk::IndexType::eUint32,
    .indexData = indexBuffer->device_address()
};

vk::AccelerationStructureGeometryKHR geometry{
    .geometryType = vk::GeometryTypeKHR::eTriangles,
    .geometry = {.triangles = triangles},
    .flags = vk::GeometryFlagBitsKHR::eOpaque
};

auto blas = BottomLevelAccelerationStructure::build(
    device, allocator, cmd, geometry, primitiveCount
);
```

### BLAS Class

```cpp
class BottomLevelAccelerationStructure {
public:
    vk::AccelerationStructureKHR handle() const;
    vk::DeviceAddress device_address() const;
};
```

## Top-Level Acceleration Structure

Contains instances referencing BLAS with transforms.

### Building TLAS

```cpp
std::vector<vk::AccelerationStructureInstanceKHR> instances;

// Add instances
instances.push_back({
    .transform = toVkTransform(modelMatrix),
    .instanceCustomIndex = 0,
    .mask = 0xFF,
    .instanceShaderBindingTableRecordOffset = 0,
    .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
    .accelerationStructureReference = blas->device_address()
});

auto tlas = TopLevelAccelerationStructure::build(
    device, allocator, cmd, instances
);
```

### Transform Conversion

```cpp
vk::TransformMatrixKHR toVkTransform(const glm::mat4& m) {
    vk::TransformMatrixKHR transform;
    // Row-major 3x4 matrix
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            transform.matrix[row][col] = m[col][row];
        }
    }
    return transform;
}
```

### TLAS Class

```cpp
class TopLevelAccelerationStructure {
public:
    vk::AccelerationStructureKHR handle() const;
    vk::DeviceAddress device_address() const;

    void update(CommandBuffer& cmd,
                std::span<const vk::AccelerationStructureInstanceKHR> instances);
};
```

### Updating TLAS

For dynamic scenes with moving objects:

```cpp
// Update instance transforms
for (size_t i = 0; i < objects.size(); ++i) {
    instances[i].transform = toVkTransform(objects[i].modelMatrix);
}

// Rebuild TLAS (can be done per-frame)
tlas->update(cmd, instances);
```

## RayTracedScene

High-level scene management:

```cpp
#include <VulkanWrapper/RayTracing/RayTracedScene.h>

RayTracedScene scene(device, allocator);

// Add meshes
scene.add_mesh(mesh1, blas1);
scene.add_mesh(mesh2, blas2);

// Add instances
scene.add_instance(mesh1, transform1);
scene.add_instance(mesh1, transform2);
scene.add_instance(mesh2, transform3);

// Build acceleration structures
scene.build(cmd);

// Get TLAS for ray tracing
auto& tlas = scene.tlas();
```

## Descriptor Binding

Bind acceleration structure to descriptor set:

```cpp
// Descriptor layout
auto layout = DescriptorSetLayoutBuilder()
    .setDevice(device)
    .addBinding(0, vk::DescriptorType::eAccelerationStructureKHR,
                vk::ShaderStageFlagBits::eRaygenKHR |
                vk::ShaderStageFlagBits::eClosestHitKHR)
    .build();

// Write descriptor
vk::WriteDescriptorSetAccelerationStructureKHR asWrite{
    .accelerationStructureCount = 1,
    .pAccelerationStructures = &tlas->handle()
};

vk::WriteDescriptorSet write{
    .pNext = &asWrite,
    .dstSet = descriptorSet,
    .dstBinding = 0,
    .descriptorCount = 1,
    .descriptorType = vk::DescriptorType::eAccelerationStructureKHR
};
```

## In Shaders

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;

void main() {
    vec3 origin = /* ray origin */;
    vec3 direction = /* ray direction */;

    traceRayEXT(
        tlas,           // Acceleration structure
        gl_RayFlagsOpaqueEXT,
        0xFF,           // Cull mask
        0,              // SBT record offset
        0,              // SBT record stride
        0,              // Miss index
        origin,
        0.001,          // tMin
        direction,
        10000.0,        // tMax
        0               // Payload location
    );
}
```

## Best Practices

1. **Build BLAS once** for static geometry
2. **Update TLAS** for dynamic instances
3. **Use opaque geometry flag** when possible
4. **Batch BLAS builds** for efficiency
5. **Consider compaction** for memory savings
