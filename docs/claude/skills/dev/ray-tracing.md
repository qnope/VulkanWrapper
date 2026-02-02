# Ray Tracing

## Setup

```cpp
// Device with RT
auto device = instance->findGpu().with_ray_tracing().build();

// Allocator with buffer device address
auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

## RayTracedScene (Recommended)

```cpp
vw::rt::RayTracedScene scene(device, allocator);

// Add instances (BLAS created automatically, deduped by geometry hash)
auto id = scene.add_instance(mesh, transform);

// Modify
scene.set_transform(id, newTransform);
scene.set_visible(id, false);

// Build
scene.build();  // Initial: BLAS + TLAS

// Update (refit TLAS only)
if (scene.needs_update()) scene.update();

// Access
vk::AccelerationStructureKHR tlas = scene.tlas_handle();
```

## Pipeline & SBT

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

## Barriers

Always barrier between AS build and trace:
```cpp
tracker.track({tlas, eBuild, eWrite});
tracker.request({tlas, eRTShader, eRead});
tracker.flush(cmd);
```
