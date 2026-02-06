# Ray Tracing

All RT code is in namespace `vw::rt`. Headers in `VulkanWrapper/include/VulkanWrapper/RayTracing/`.

## Setup

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .with_ray_tracing()    // Enables RT pipeline, AS, buffer device address
    .build();

auto allocator = AllocatorBuilder(instance, device).build();
```

## RayTracedScene (Recommended High-Level API)

Manages BLAS/TLAS lifecycle, deduplicates geometry by mesh hash:

```cpp
vw::rt::RayTracedScene scene(device, allocator);

// Add instances (BLAS created automatically, deduped by geometry hash)
auto id = scene.add_instance(mesh, transform);

// Modify instances
scene.set_transform(id, newTransform);
scene.set_visible(id, false);

// Build (initial: creates BLAS + TLAS)
scene.build();

// Update (refit TLAS only -- for transform/visibility changes)
if (scene.needs_update()) scene.update();

// Access TLAS for descriptor binding
vk::AccelerationStructureKHR tlas = scene.tlas_handle();
```

## Low-Level API (BLAS/TLAS Builders)

For custom control over acceleration structure construction:

```cpp
auto blas = vw::rt::BottomLevelAccelerationStructureBuilder(device, allocator)
    .add_geometry(vertexBuffer, indexBuffer, vertexCount, indexCount, stride)
    .build();

auto tlas = vw::rt::TopLevelAccelerationStructureBuilder(device, allocator)
    .add_instance(blas, transform)
    .build();
```

## Pipeline & Shader Binding Table

```cpp
auto pipeline = vw::rt::RayTracingPipelineBuilder(device, allocator, layout)
    .set_ray_generation_shader(rgen)
    .add_miss_shader(rmiss)
    .add_closest_hit_shader(rchit)
    .build();

vw::rt::ShaderBindingTable sbt(allocator, pipeline.ray_generation_handle());
for (auto& h : pipeline.miss_handles()) sbt.add_miss_record(h);
for (auto& h : pipeline.closest_hit_handles()) sbt.add_hit_record(h);

cmd.traceRaysKHR(sbt.raygen_region(), sbt.miss_region(), sbt.hit_region(), {}, w, h, 1);
```

## GeometryReference

Provides shader access to geometry data via `GeometryReference` (`RayTracing/GeometryReference.h`):

```glsl
// In GLSL: use geometry_access.glsl for vertex/index access in hit shaders
#include "geometry_access.glsl"
```

Exposes vertex positions, normals, texture coordinates and index data to ray tracing hit shaders. Includes transform matrix support.

## Barriers

Always barrier between AS build and trace:
```cpp
tracker.track(AccelerationStructureState{tlas, eAccelerationStructureBuildKHR, eAccelerationStructureWriteKHR});
tracker.request(AccelerationStructureState{tlas, eRayTracingShaderKHR, eAccelerationStructureReadKHR});
tracker.flush(cmd);
```

## Integration with Render Passes

RT results are typically consumed by a ScreenSpacePass (e.g., `IndirectLightPass`):
1. Build/update `RayTracedScene` -> barrier -> trace rays -> write to storage image
2. Barrier storage image to `eShaderReadOnlyOptimal`
3. `ScreenSpacePass` samples the RT result as input texture

See `examples/Advanced/` for a complete deferred rendering + RT pipeline.

## Testing

Define `get_ray_tracing_gpu()` locally in each RT test file (not in a shared header):
```cpp
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```
See `VulkanWrapper/tests/RayTracing/` for examples.
