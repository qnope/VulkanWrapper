# RayTracing Module

`vw::rt` namespace · `RayTracing/` directory · Tier 6

Hardware ray tracing built on `VK_KHR_ray_tracing_pipeline` and `VK_KHR_acceleration_structure`.

## RayTracedScene

High-level scene manager combining ray tracing and rasterization:

```cpp
vw::rt::RayTracedScene rayTracedScene(device, allocator);
auto id = rayTracedScene.add_instance(mesh, transform);  // registers geometry + creates BLAS
rayTracedScene.set_material_sbt_mapping(manager.sbt_mapping());
rayTracedScene.build();  // builds all BLAS + TLAS
```

- `add_instance(mesh, transform)` → `InstanceId` — deduplicates geometry (same mesh → shared BLAS)
- `set_transform()`, `set_visible()`, `remove_instance()` — per-instance control
- `update()` — rebuilds TLAS for transform changes (without rebuilding BLAS)
- `scene()` — access embedded `Scene` for rasterization
- `tlas_handle()` / `tlas()` — acceleration structure for shader binding
- `geometry_buffer()` — GPU buffer with per-geometry vertex/index addresses

## BottomLevelAccelerationStructure (BLAS)

Per-mesh geometry acceleration structure. `BottomLevelAccelerationStructureList` manages multiple BLAS built together for efficiency.

## TopLevelAccelerationStructure (TLAS)

Scene-level acceleration structure with per-instance transforms, SBT offsets, and visibility flags.

## RayTracingPipeline / RayTracingPipelineBuilder

```cpp
auto pipeline = RayTracingPipelineBuilder(device, allocator, layout)
    .set_ray_generation_shader(rgenModule)
    .add_miss_shader(missModule)
    .add_closest_hit_shader(chitModule)
    .build();
```

Provides shader handle extraction (`ray_generation_handle()`, `miss_handles()`, `closest_hit_handles()`) for SBT construction.

## ShaderBindingTable (SBT)

Maps ray types to shader groups (raygen, miss, closest hit). Built from RayTracingPipeline handles.

## GeometryReference / GeometryReferenceBuffer

GPU-accessible buffer containing per-geometry vertex/index buffer device addresses. Used in closest hit shaders (`geometry_access.glsl`) for vertex attribute access during ray tracing.
