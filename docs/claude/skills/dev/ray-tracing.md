# Ray Tracing

Hardware ray tracing using Vulkan's `VK_KHR_ray_tracing_pipeline` extension suite.

## Prerequisites

```cpp
// 1. Enable ray tracing during device creation
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .with_ray_tracing()  // Enables RT extensions
    .build();

// 2. Enable buffer device address for allocator
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/RayTracing/`

---

## Acceleration Structures Overview

Ray tracing uses a two-level hierarchy:

1. **BLAS (Bottom-Level)**: Contains geometry (triangles, AABBs)
   - One BLAS per unique mesh geometry
   - Shared across instances with same geometry

2. **TLAS (Top-Level)**: Contains instances of BLASes
   - Each instance has a transform and BLAS reference
   - Rebuilt when instances change

```
TLAS
├── Instance 0 (BLAS 0, transform A)
├── Instance 1 (BLAS 0, transform B)  ← Same geometry, different transform
├── Instance 2 (BLAS 1, transform C)
└── Instance 3 (BLAS 2, transform D)
```

---

## Using RayTracedScene (Recommended)

`RayTracedScene` manages the full RT infrastructure:

```cpp
#include "VulkanWrapper/RayTracing/RayTracedScene.h"

vw::rt::RayTracedScene scene(device, allocator);

// Add mesh instances (BLAS created automatically)
auto id1 = scene.add_instance(mesh, glm::mat4(1.0f));
auto id2 = scene.add_instance(mesh, glm::translate(glm::vec3(5, 0, 0)));
auto id3 = scene.add_instance(otherMesh, transform);

// Modify instances
scene.set_transform(id1, newTransform);
scene.set_visible(id2, false);         // Hide without removing
scene.set_sbt_offset(id3, 1);          // Shader binding table offset
scene.set_custom_index(id1, 42);       // Custom data for shaders

// Build acceleration structures
scene.build();  // Initial build (BLAS + TLAS)

// Later, update if transforms changed
if (scene.needs_update()) {
    scene.update();  // Refit TLAS only
}

// Access for rendering
vk::AccelerationStructureKHR tlas = scene.tlas_handle();
vk::DeviceAddress tlasAddress = scene.tlas_device_address();

// Access embedded Scene for rasterization
const Model::Scene& rasterScene = scene.scene();
```

### Geometry Deduplication

`RayTracedScene` automatically deduplicates BLAS by mesh geometry hash:

```cpp
// These share the same BLAS (same geometry)
auto id1 = scene.add_instance(meshA, transform1);
auto id2 = scene.add_instance(meshA, transform2);

// This creates a new BLAS (different geometry)
auto id3 = scene.add_instance(meshB, transform3);
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/RayTracing/RayTracedScene.h`

---

## Manual BLAS/TLAS Creation

For more control, use the builder classes directly:

### Building BLAS

```cpp
#include "VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h"

// Create list to hold BLASes and manage buffers
vw::rt::as::BottomLevelAccelerationStructureList blasList(device, allocator);

// Build BLAS from mesh
auto& blas = vw::rt::as::BottomLevelAccelerationStructureBuilder(device)
    .add_mesh(mesh)  // Helper for triangle meshes
    .build_into(blasList);

// Or manually specify geometry
auto& blas2 = vw::rt::as::BottomLevelAccelerationStructureBuilder(device)
    .add_geometry(geometryKHR, buildRangeInfo)
    .build_into(blasList);

// Submit and wait for build
blasList.submit_and_wait();

// Get addresses for TLAS
std::vector<vk::DeviceAddress> addresses = blasList.device_addresses();
```

### Building TLAS

```cpp
#include "VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h"

vw::rt::as::TopLevelAccelerationStructureBuilder tlasBuilder(device, allocator);

// Add BLAS instances with transforms
for (size_t i = 0; i < blasAddresses.size(); ++i) {
    tlasBuilder.add_bottom_level_acceleration_structure_address(
        blasAddresses[i],
        transforms[i]
    );
}

// Build TLAS
auto tlas = tlasBuilder.build(commandBuffer);
```

---

## Ray Tracing Pipeline

### Pipeline Creation

```cpp
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

// Create pipeline layout
auto layout = PipelineLayoutBuilder(device)
    .add_descriptor_set_layout(descriptorLayout)
    .add_push_constant_range<PushConstants>(vk::ShaderStageFlagBits::eRaygenKHR)
    .build();

// Compile shaders
auto compiler = ShaderCompiler()
    .add_include_path("shaders/include");

auto rgen = compiler.compile_file_to_module(device, "shaders/raytrace.rgen");
auto rmiss = compiler.compile_file_to_module(device, "shaders/raytrace.rmiss");
auto rchit = compiler.compile_file_to_module(device, "shaders/raytrace.rchit");

// Build pipeline
auto pipeline = vw::rt::RayTracingPipelineBuilder(device, allocator, std::move(layout))
    .set_ray_generation_shader(rgen)
    .add_miss_shader(rmiss)
    .add_closest_hit_shader(rchit)
    .build();
```

### Shader Binding Table

```cpp
#include "VulkanWrapper/RayTracing/ShaderBindingTable.h"

// Create SBT from pipeline handles
vw::rt::ShaderBindingTable sbt(allocator, pipeline.ray_generation_handle());

// Add miss shader records
for (const auto& handle : pipeline.miss_handles()) {
    sbt.add_miss_record(handle);
}

// Add hit shader records (can include per-instance data)
struct HitData { uint32_t materialId; };
for (const auto& handle : pipeline.closest_hit_handles()) {
    sbt.add_hit_record(handle, HitData{materialIndex});
}

// Get regions for traceRays
vk::StridedDeviceAddressRegionKHR raygenRegion = sbt.raygen_region();
vk::StridedDeviceAddressRegionKHR missRegion = sbt.miss_region();
vk::StridedDeviceAddressRegionKHR hitRegion = sbt.hit_region();
vk::StridedDeviceAddressRegionKHR callableRegion{};  // Empty if not used
```

### Dispatching Rays

```cpp
cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.handle());
cmd.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                       pipeline.handle_layout(), 0, descriptorSet, {});

cmd.traceRaysKHR(
    raygenRegion,
    missRegion,
    hitRegion,
    callableRegion,
    width, height, 1  // Dispatch dimensions
);
```

---

## Shader Structure

### Ray Generation Shader (.rgen)

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;
layout(set = 0, binding = 1, rgba32f) uniform image2D outputImage;

layout(location = 0) rayPayloadEXT vec3 hitValue;

void main() {
    vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    vec2 uv = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec3 origin = camera_position;
    vec3 direction = compute_ray_direction(uv);

    traceRayEXT(tlas,
        gl_RayFlagsOpaqueEXT,  // Ray flags
        0xFF,                   // Cull mask
        0,                      // SBT record offset
        0,                      // SBT record stride
        0,                      // Miss shader index
        origin,
        0.001,                  // tMin
        direction,
        10000.0,                // tMax
        0                       // Payload location
    );

    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 1.0));
}
```

### Miss Shader (.rmiss)

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main() {
    hitValue = vec3(0.0, 0.0, 0.2);  // Background color
}
```

### Closest Hit Shader (.rchit)

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

hitAttributeEXT vec2 baryCoord;

void main() {
    // Access hit information
    vec3 bary = vec3(1.0 - baryCoord.x - baryCoord.y, baryCoord.x, baryCoord.y);

    // gl_InstanceCustomIndexEXT - custom index set via set_custom_index()
    // gl_PrimitiveID - triangle index within mesh
    // gl_HitTEXT - ray parameter at intersection

    hitValue = bary;  // Visualize barycentric coordinates
}
```

---

## Barrier Management

### After BLAS Build

```cpp
tracker.track(vw::Barrier::AccelerationStructureState{
    .handle = blas.handle(),
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR
});
tracker.request(vw::Barrier::AccelerationStructureState{
    .handle = blas.handle(),
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
```

### After TLAS Build, Before Ray Tracing

```cpp
tracker.track(vw::Barrier::AccelerationStructureState{
    .handle = tlas.handle(),
    .stage = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureWriteKHR
});
tracker.request(vw::Barrier::AccelerationStructureState{
    .handle = tlas.handle(),
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
});
tracker.flush(cmd);
```

### RT Output Image to Shader Read

```cpp
// After ray tracing writes to storage image
tracker.track(vw::Barrier::ImageState{
    .image = outputImage,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eGeneral,
    .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
    .access = vk::AccessFlagBits2::eShaderStorageWrite
});
tracker.request(vw::Barrier::ImageState{
    .image = outputImage,
    .subresourceRange = range,
    .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .stage = vk::PipelineStageFlagBits2::eFragmentShader,
    .access = vk::AccessFlagBits2::eShaderSampledRead
});
```

---

## Complete Example

```cpp
// Setup
vw::rt::RayTracedScene scene(device, allocator);
scene.add_instance(mesh1, transform1);
scene.add_instance(mesh2, transform2);
scene.build();

// Create pipeline and SBT
auto pipeline = createRTPipeline(device, allocator);
vw::rt::ShaderBindingTable sbt(allocator, pipeline.ray_generation_handle());
for (auto& h : pipeline.miss_handles()) sbt.add_miss_record(h);
for (auto& h : pipeline.closest_hit_handles()) sbt.add_hit_record(h);

// Render loop
{
    CommandBufferRecorder recorder(cmd);
    vw::Barrier::ResourceTracker tracker;

    // Update transforms if needed
    scene.set_transform(id, newTransform);
    if (scene.needs_update()) {
        scene.update();
    }

    // Barrier: TLAS ready for ray tracing
    tracker.track(vw::Barrier::AccelerationStructureState{...});
    tracker.request(vw::Barrier::AccelerationStructureState{
        .handle = scene.tlas_handle(),
        .stage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
        .access = vk::AccessFlagBits2::eAccelerationStructureReadKHR
    });
    tracker.flush(cmd);

    // Dispatch rays
    cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, pipeline.handle());
    cmd.bindDescriptorSets(...);
    cmd.traceRaysKHR(
        sbt.raygen_region(), sbt.miss_region(), sbt.hit_region(), {},
        width, height, 1
    );

    // Transition output for display
    tracker.track(...);
    tracker.request(...);
    tracker.flush(cmd);
}
```

---

## Anti-Patterns

```cpp
// DON'T: Forget buffer device address flag
auto allocator = AllocatorBuilder(instance, device).build();
// RT buffers will fail!

// DO: Enable buffer device address
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();

// DON'T: Rebuild TLAS when only transforms changed
scene.build();  // Full rebuild, expensive

// DO: Update TLAS (refit) when possible
if (scene.needs_update()) {
    scene.update();  // Fast refit
}

// DON'T: Forget barriers between AS build and trace
cmd.buildAccelerationStructuresKHR(...);
cmd.traceRaysKHR(...);  // Race condition!

// DO: Barrier between build and trace
cmd.buildAccelerationStructuresKHR(...);
tracker.request(AccelerationStructureState{...});
tracker.flush(cmd);
cmd.traceRaysKHR(...);
```
