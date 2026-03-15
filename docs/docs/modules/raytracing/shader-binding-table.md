---
sidebar_position: 3
---

# Shader Binding Table

The Shader Binding Table (SBT) tells the GPU which shaders to invoke during
ray tracing. When a ray hits geometry, misses all geometry, or is first
generated, the SBT determines which shader runs. VulkanWrapper provides the
`ShaderBindingTable` class (in namespace `vw::rt`) to construct and manage
the SBT from `RayTracingPipeline` handles.

## How the SBT Works

The SBT is a GPU buffer organized into three regions:

```
+------------------------------+
|     Ray Generation (1)       |  <- One entry: the raygen shader
+------------------------------+
|     Miss Shaders (N)         |  <- One entry per miss shader
+------------------------------+
|     Hit Groups (M)           |  <- One entry per closest-hit shader
+------------------------------+
```

Each entry is a fixed-size **record** (256 bytes) containing the shader group
handle and optional payload data. When you call `traceRaysKHR`, you pass the
device address and stride of each region.

### Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `ShaderBindingTableHandleRecordSize` | 256 | Size of each SBT record in bytes |
| `ShaderBindingTableHandleSizeAlignment` | 64 | Alignment of shader group handles |
| `MaximumRecordInShaderBindingTable` | 4096 | Maximum records per region buffer |

## Creating a ShaderBindingTable

### Step 1: Build the RayTracingPipeline

The SBT is populated from a `RayTracingPipeline`. First, build the pipeline
using `RayTracingPipelineBuilder`:

```cpp
auto pipelineLayout = /* ... */;

auto pipeline = vw::rt::RayTracingPipelineBuilder(device, allocator, pipelineLayout)
    .set_ray_generation_shader(raygenModule)
    .add_miss_shader(missModule)
    .add_closest_hit_shader(closestHitModule)
    .build();
```

The pipeline stores the compiled shader group handles internally.

### Step 2: Construct the SBT

Pass the allocator and the raygen handle to the `ShaderBindingTable`
constructor:

```cpp
auto sbt = vw::rt::ShaderBindingTable(
    allocator, pipeline.ray_generation_handle());
```

The constructor writes the raygen record into the SBT buffer immediately.

### Step 3: Add Miss and Hit Records

Retrieve handles from the pipeline and add them to the SBT:

```cpp
// Add miss shader records
for (const auto &handle : pipeline.miss_handles()) {
    sbt.add_miss_record(handle);
}

// Add closest-hit shader records
for (const auto &handle : pipeline.closest_hit_handles()) {
    sbt.add_hit_record(handle);
}
```

### Complete Example

```cpp
// Build pipeline
auto pipeline = vw::rt::RayTracingPipelineBuilder(device, allocator, layout)
    .set_ray_generation_shader(raygenModule)
    .add_miss_shader(primaryMissModule)
    .add_miss_shader(shadowMissModule)
    .add_closest_hit_shader(opaqueHitModule)
    .add_closest_hit_shader(transparentHitModule)
    .build();

// Build SBT
auto sbt = vw::rt::ShaderBindingTable(
    allocator, pipeline.ray_generation_handle());

for (const auto &handle : pipeline.miss_handles()) {
    sbt.add_miss_record(handle);
}

for (const auto &handle : pipeline.closest_hit_handles()) {
    sbt.add_hit_record(handle);
}
```

## SBT Record Payload

Both `add_miss_record` and `add_hit_record` are variadic templates that
accept optional payload data after the handle:

```cpp
void add_miss_record(const ShaderBindingTableHandle &handle,
                     const auto &...object);

void add_hit_record(const ShaderBindingTableHandle &handle,
                    const auto &...object);
```

The payload is appended after the shader group handle within the 256-byte
record. The payload must fit:
`sizeof(payload) + ShaderBindingTableHandleSizeAlignment < ShaderBindingTableHandleRecordSize`.

### Example: Embedding Material Data

```cpp
struct HitPayload {
    uint64_t materialBufferAddress;
    uint32_t materialIndex;
};

HitPayload payload{
    .materialBufferAddress = materialBuffer.device_address(),
    .materialIndex = 42
};

sbt.add_hit_record(hitHandle, payload);
```

In the shader, access the payload via `shaderRecordEXT`:

```glsl
layout(shaderRecordEXT) buffer SbtData {
    uint64_t materialBufferAddress;
    uint    materialIndex;
} sbtData;
```

## Dispatching Rays

Use the three region accessors to fill the `traceRaysKHR` call:

```cpp
// Each returns a vk::StridedDeviceAddressRegionKHR
// with deviceAddress, stride, and size fields set.
auto raygenRegion = sbt.raygen_region();
auto missRegion   = sbt.miss_region();
auto hitRegion    = sbt.hit_region();

// An empty region for callable shaders (not used here)
vk::StridedDeviceAddressRegionKHR callableRegion{};

// Dispatch rays
cmd.traceRaysKHR(
    raygenRegion,
    missRegion,
    hitRegion,
    callableRegion,
    width, height, 1);
```

The library also provides `CommandBufferRecorder::traceRaysKHR` as a
convenience wrapper:

```cpp
vw::CommandBufferRecorder recorder(cmd);
recorder.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    vk::StridedDeviceAddressRegionKHR{},
    width, height, 1);
```

## Integration with RayTracedScene

`RayTracedScene` manages BLAS/TLAS construction and geometry registration.
Each instance in the scene can have a per-instance SBT offset that selects
which hit group to use:

```cpp
vw::rt::RayTracedScene scene(device, allocator);

// Add mesh instances
auto id = scene.add_instance(mesh, transform);

// Set SBT offset for this instance (selects the hit group)
scene.set_sbt_offset(id, 1); // Use hit group at index 1
```

### Material-to-SBT Mapping

For scenes with multiple material types, `RayTracedScene` supports automatic
SBT offset assignment based on material type:

```cpp
// Map material types to SBT hit group indices
std::unordered_map<vw::Model::Material::MaterialTypeTag, uint32_t> mapping;
mapping[texturedMaterialTag] = 0;   // Textured materials use hit group 0
mapping[coloredMaterialTag]  = 1;   // Colored materials use hit group 1

scene.set_material_sbt_mapping(mapping);
```

When instances are added, their SBT offset is automatically set based on
their material type.

### Complete Ray Tracing Setup

Here is how the pieces fit together, following the pattern used in
`IndirectLightPass`:

```cpp
// 1. Build ray tracing pipeline with shaders for each material type
auto builder = vw::rt::RayTracingPipelineBuilder(device, allocator, layout);
builder.set_ray_generation_shader(raygenModule);
builder.add_miss_shader(missModule);

// One closest-hit shader per material type
for (const auto &handler : materialHandlers) {
    auto hitModule = compiler.compile_to_module(
        device, handler->shader_source(),
        vk::ShaderStageFlagBits::eClosestHitKHR,
        "material.rchit");
    builder.add_closest_hit_shader(hitModule);
}
auto pipeline = builder.build();

// 2. Build SBT from pipeline handles
auto sbt = vw::rt::ShaderBindingTable(
    allocator, pipeline.ray_generation_handle());

auto missHandles = pipeline.miss_handles();
for (const auto &handle : missHandles) {
    sbt.add_miss_record(handle);
}

auto hitHandles = pipeline.closest_hit_handles();
for (const auto &handle : hitHandles) {
    sbt.add_hit_record(handle);
}

// 3. Build acceleration structures
scene.build();

// 4. Trace rays
cmd.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    vk::StridedDeviceAddressRegionKHR{},
    width, height, 1);
```

## SBT Indexing in Shaders

Understanding how `traceRayEXT` selects shaders is essential for
multi-material and multi-ray-type setups.

### Miss Shader Selection

The `missIndex` parameter of `traceRayEXT` directly indexes into the miss
region:

```glsl
// Use miss shader 0 for primary rays
traceRayEXT(tlas, flags, 0xFF,
            0, 0,
            0,      // missIndex -> miss shader 0
            origin, tMin, direction, tMax, 0);

// Use miss shader 1 for shadow rays
traceRayEXT(tlas, flags, 0xFF,
            0, 0,
            1,      // missIndex -> miss shader 1
            origin, tMin, direction, tMax, 1);
```

### Hit Group Selection

The hit group index is computed as:

```
hitGroupIndex = instanceShaderBindingTableRecordOffset
              + (sbtRecordOffset + geometryIndex * sbtRecordStride)
```

- `instanceShaderBindingTableRecordOffset` is set per-instance via
  `scene.set_sbt_offset(id, offset)` or through the material-to-SBT
  mapping.
- `sbtRecordOffset` and `sbtRecordStride` are parameters of
  `traceRayEXT`.
- `geometryIndex` is the index of the geometry within the BLAS.

For a simple setup with one geometry per BLAS and material-based hit group
selection, set the instance SBT offset to the desired hit group index.

## Internal Buffer Layout

The SBT uses two separate GPU buffers internally:

1. **Raygen + Miss buffer**: The raygen record is at offset 0, followed by
   miss records. The `raygen_region()` points to offset 0 with size = 1
   record. The `miss_region()` points to offset 256 (after raygen) with
   size = N miss records.

2. **Hit buffer**: A separate buffer holding all hit group records.
   `hit_region()` points to the start of this buffer.

Both buffers have `eShaderBindingTableKHR | eShaderDeviceAddress` usage flags
and are host-visible for direct CPU writes.

## Best Practices

1. **Build the SBT after the pipeline** -- you need the pipeline's compiled
   shader group handles.
2. **Add miss records before hit records** -- while not strictly required by
   the API, this matches the logical pipeline layout.
3. **Use `set_material_sbt_mapping`** on `RayTracedScene` for automatic
   per-material hit group assignment.
4. **Keep payload small** -- each record is 256 bytes total including the
   64-byte handle. Prefer GPU buffer addresses over inline data.
5. **Match miss shader indices** in `traceRayEXT` to the order you called
   `add_miss_shader` on the pipeline builder.
