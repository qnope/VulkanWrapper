---
sidebar_position: 2
title: "Ray Tracing Pipeline"
---

# Ray Tracing Pipeline

VulkanWrapper provides `RayTracingPipelineBuilder` and `RayTracingPipeline` for creating and using Vulkan KHR ray tracing pipelines. These complement the `RayTracedScene` by defining the shader programs that execute when rays hit (or miss) scene geometry.

## Ray Tracing Shader Types

A Vulkan ray tracing pipeline uses three types of shaders:

| Shader Type | Purpose | When It Runs |
|-------------|---------|--------------|
| **Ray Generation** | Launches rays. Typically one per pixel. | Once per dispatch invocation |
| **Miss** | Handles rays that hit nothing. Returns background color or sky radiance. | When `traceRayEXT()` finds no intersection |
| **Closest Hit** | Handles rays that hit geometry. Reads material data, computes shading. | When `traceRayEXT()` finds the closest intersection |

## RayTracingPipelineBuilder

The builder constructs a `RayTracingPipeline` from compiled shader modules.

### Constructor

```cpp
RayTracingPipelineBuilder(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator,
    PipelineLayout pipelineLayout);
```

The `PipelineLayout` describes the descriptor set layouts and push constant ranges that the shaders use.

### Setting Shaders

```cpp
// Set the single ray generation shader (required, exactly one)
RayTracingPipelineBuilder &
set_ray_generation_shader(
    std::shared_ptr<const ShaderModule> module);

// Add a miss shader (at least one required)
RayTracingPipelineBuilder &
add_miss_shader(std::shared_ptr<const ShaderModule> module);

// Add a closest-hit shader (at least one required)
RayTracingPipelineBuilder &
add_closest_hit_shader(
    std::shared_ptr<const ShaderModule> module);
```

You can add **multiple** miss and closest-hit shaders. The order in which you add them determines their index in the Shader Binding Table.

A common pattern is one closest-hit shader per material type:

```cpp
// One closest-hit shader per material type, in SBT order
for (auto [tag, handler] : mat_mgr.ordered_handlers()) {
    auto shader = compiler.compile_file_to_module(
        device, handler->brdf_path());
    builder.add_closest_hit_shader(shader);
}
```

### Building

```cpp
RayTracingPipeline build();
```

Returns the compiled `RayTracingPipeline`. This call compiles all shader stages and creates the Vulkan pipeline object.

## RayTracingPipeline

`RayTracingPipeline` inherits from `ObjectWithUniqueHandle<vk::UniquePipeline>` and provides access to the pipeline layout and shader handles needed for the Shader Binding Table.

### Pipeline Layout

```cpp
const PipelineLayout &layout() const noexcept;
```

Returns the pipeline layout used to create the pipeline. Needed when binding descriptor sets and pushing constants.

### Shader Handles

After building the pipeline, you can retrieve the shader group handles. These are opaque byte arrays that the GPU uses to identify which shader to run:

```cpp
// Ray generation shader handle
ShaderBindingTableHandle ray_generation_handle() const;

// All miss shader handles (in order of add_miss_shader calls)
std::span<const ShaderBindingTableHandle> miss_handles() const;

// All closest-hit shader handles (in order of add_closest_hit_shader calls)
std::span<const ShaderBindingTableHandle> closest_hit_handles() const;
```

`ShaderBindingTableHandle` is defined as:

```cpp
using ShaderBindingTableHandle = std::vector<std::byte>;
```

These handles are passed to the `ShaderBindingTable` to build the SBT buffer.

## ShaderBindingTable

The `ShaderBindingTable` maps shader handles to GPU memory regions that the ray tracing dispatch reads. It organizes handles into three regions: ray generation, miss, and hit.

### Constructor

```cpp
ShaderBindingTable(
    std::shared_ptr<const Allocator> allocator,
    const ShaderBindingTableHandle &raygen_handle);
```

The ray generation handle is provided at construction because there is always exactly one.

### Adding Records

```cpp
// Add a miss shader record (optionally with inline data)
void add_miss_record(const ShaderBindingTableHandle &handle,
                     const auto &...object);

// Add a hit shader record (optionally with inline data)
void add_hit_record(const ShaderBindingTableHandle &handle,
                    const auto &...object);
```

Each record consists of a shader handle followed by optional inline data (shader record data). The inline data is accessible in the shader via `shaderRecordEXT`.

Records must be added in the same order as the corresponding shaders were added to the pipeline builder.

### Getting Regions for Dispatch

```cpp
vk::StridedDeviceAddressRegionKHR raygen_region() const;
vk::StridedDeviceAddressRegionKHR miss_region() const;
vk::StridedDeviceAddressRegionKHR hit_region() const;
```

These return the GPU address regions needed by `vkCmdTraceRaysKHR`:

```cpp
cmd.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    vk::StridedDeviceAddressRegionKHR{},  // callable (unused)
    width, height, 1);
```

### SBT Record Layout

Each SBT record is aligned to 64 bytes (`ShaderBindingTableHandleSizeAlignment`) and has a maximum size of 256 bytes (`ShaderBindingTableHandleRecordSize`):

```cpp
constexpr uint64_t ShaderBindingTableHandleSizeAlignment = 64;
constexpr uint64_t ShaderBindingTableHandleRecordSize = 256;
constexpr uint64_t MaximumRecordInShaderBindingTable = 4'096;
```

The `ShaderBindingTableRecord` struct holds the handle bytes plus optional inline data:

```cpp
struct alignas(64) ShaderBindingTableRecord {
    // Constructed from handle only
    ShaderBindingTableRecord(std::span<const std::byte> handle);

    // Constructed from handle + inline data object
    ShaderBindingTableRecord(std::span<const std::byte> handle,
                             const auto &object);

    std::array<std::byte, 256> data{};
};
```

## Complete Example

Here is a full example of creating a ray tracing pipeline with one miss shader and multiple closest-hit shaders (one per material type):

```cpp
vw::ShaderCompiler compiler;

// Compile shaders
auto raygen = compiler.compile_file_to_module(
    device, shader_dir / "indirect.rgen");
auto miss = compiler.compile_file_to_module(
    device, shader_dir / "indirect.rmiss");

// Create descriptor set layout for the pipeline
auto desc_layout = vw::DescriptorSetLayoutBuilder(device)
    .add_binding(vk::DescriptorType::eAccelerationStructureKHR,
                 vk::ShaderStageFlagBits::eRayGenerationKHR)
    .add_binding(vk::DescriptorType::eStorageImage,
                 vk::ShaderStageFlagBits::eRayGenerationKHR)
    // ... more bindings ...
    .build();

// Create pipeline layout
vk::PushConstantRange push_range{
    vk::ShaderStageFlagBits::eRayGenerationKHR |
    vk::ShaderStageFlagBits::eMissKHR |
    vk::ShaderStageFlagBits::eClosestHitKHR,
    0, sizeof(vw::IndirectLightPushConstants)};

vw::PipelineLayout layout(device, {desc_layout}, {push_range});

// Build the ray tracing pipeline
vw::rt::RayTracingPipelineBuilder builder(
    device, allocator, layout);

builder.set_ray_generation_shader(raygen);
builder.add_miss_shader(miss);

// Add one closest-hit shader per material type
for (auto [tag, handler] :
     material_manager.ordered_handlers()) {
    auto hit_shader = compiler.compile_file_to_module(
        device, handler->brdf_path());
    builder.add_closest_hit_shader(hit_shader);
}

auto pipeline = builder.build();

// Build the Shader Binding Table
vw::rt::ShaderBindingTable sbt(
    allocator, pipeline.ray_generation_handle());

// Add miss records
for (const auto &handle : pipeline.miss_handles()) {
    sbt.add_miss_record(handle);
}

// Add hit records
for (const auto &handle : pipeline.closest_hit_handles()) {
    sbt.add_hit_record(handle);
}

// Dispatch ray tracing
cmd.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,
                 pipeline.handle());
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eRayTracingKHR,
    pipeline.layout().handle(),
    0, 1, &descriptor_set, 0, nullptr);

cmd.pushConstants(
    pipeline.layout().handle(),
    vk::ShaderStageFlagBits::eRayGenerationKHR |
    vk::ShaderStageFlagBits::eMissKHR |
    vk::ShaderStageFlagBits::eClosestHitKHR,
    0, sizeof(push_constants), &push_constants);

cmd.traceRaysKHR(
    sbt.raygen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    vk::StridedDeviceAddressRegionKHR{},
    width, height, 1);
```

## SBT and Material Mapping

The connection between materials and closest-hit shaders works through SBT offsets:

1. `BindlessMaterialManager::ordered_handlers()` returns handlers in a deterministic order.
2. `BindlessMaterialManager::sbt_mapping()` maps each `MaterialTypeTag` to its index in that order.
3. `RayTracedScene::set_material_sbt_mapping()` uses this mapping to assign SBT offsets to each instance.
4. Closest-hit shaders are added to the pipeline builder in the same order as `ordered_handlers()`.
5. Hit records are added to the SBT in the same order.

This ensures that when a ray hits geometry with material type X, the GPU invokes the closest-hit shader at SBT offset X.

```cpp
// These three must use the same ordering:
auto ordered = mat_mgr.ordered_handlers();

// 1. Pipeline: add closest-hit shaders in order
for (auto [tag, handler] : ordered) {
    builder.add_closest_hit_shader(
        compiler.compile_file_to_module(
            device, handler->brdf_path()));
}

// 2. SBT: add hit records in the same order
auto hit_handles = pipeline.closest_hit_handles();
for (size_t i = 0; i < hit_handles.size(); ++i) {
    sbt.add_hit_record(hit_handles[i]);
}

// 3. Scene: set material-to-SBT-offset mapping
rt_scene.set_material_sbt_mapping(mat_mgr.sbt_mapping());
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RayTracing/RayTracingPipeline.h` | `RayTracingPipelineBuilder`, `RayTracingPipeline`, `ShaderBindingTableHandle` |
| `VulkanWrapper/RayTracing/ShaderBindingTable.h` | `ShaderBindingTable`, `ShaderBindingTableRecord` |
