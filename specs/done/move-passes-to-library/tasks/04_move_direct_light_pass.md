# Task 4: Move DirectLightPass to Library

## Summary

Move `DirectLightPass` from `examples/Advanced/DirectLightPass.h` (header-only, ~430 lines) into the VulkanWrapper library as `RenderPass/DirectLightPass.h` + `DirectLightPass.cpp`. This is the most complex pass: it fills a 7-slot G-Buffer with geometry data and computes per-fragment direct sun lighting with ray-traced shadows. Move associated shaders to the library.

The `DirectLightPassOutput` struct is removed — outputs are accessed via the slot system.

---

## Implementation Steps

### 4.1 Move shaders

**From → To:**
| Source | Destination |
|--------|-------------|
| `examples/Advanced/Shaders/GBuffer/gbuffer.vert` | `VulkanWrapper/Shaders/GBuffer/gbuffer.vert` |
| `examples/Advanced/Shaders/GBuffer/gbuffer_base.glsl` | `VulkanWrapper/Shaders/GBuffer/gbuffer_base.glsl` |

The `gbuffer_base.glsl` shader includes:
- `include/random.glsl` — already in `VulkanWrapper/Shaders/include/`
- `include/atmosphere_params.glsl` — already in `VulkanWrapper/Shaders/include/`
- `include/sun_lighting_computation.glsl` — already in `VulkanWrapper/Shaders/include/`

No include path changes needed if `ShaderCompiler` include directories are configured to resolve `VulkanWrapper/Shaders/include/`.

### 4.2 Create `DirectLightPass.h`

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/DirectLightPass.h`

```cpp
namespace vw {

class DirectLightPass : public RenderPass {
public:
    struct Formats {
        vk::Format albedo = vk::Format::eR8G8B8A8Unorm;
        vk::Format normal = vk::Format::eR32G32B32A32Sfloat;
        vk::Format tangent = vk::Format::eR32G32B32A32Sfloat;
        vk::Format bitangent = vk::Format::eR32G32B32A32Sfloat;
        vk::Format position = vk::Format::eR32G32B32A32Sfloat;
        vk::Format direct_light = vk::Format::eR16G16B16A16Sfloat;
        vk::Format indirect_ray = vk::Format::eR32G32B32A32Sfloat;
        vk::Format depth = vk::Format::eD32Sfloat;
    };

    DirectLightPass(std::shared_ptr<Device> device,
                    std::shared_ptr<Allocator> allocator,
                    const std::filesystem::path& shader_dir,
                    const rt::RayTracedScene& ray_traced_scene,
                    const Model::Material::BindlessMaterialManager& material_manager,
                    Formats formats = {});

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    // Configuration setters
    void set_uniform_buffer(const BufferBase& ubo);
    void set_sky_parameters(const SkyParameters& params);
    void set_camera_position(const glm::vec3& pos);
    void set_frame_count(uint32_t count);

private:
    Formats m_formats;
    const rt::RayTracedScene* m_ray_traced_scene;

    // Pipelines (one per material type via MeshRenderer)
    MeshRenderer m_mesh_renderer;

    // Descriptor resources
    std::shared_ptr<const DescriptorSetLayout> m_descriptor_set_layout;
    std::shared_ptr<const PipelineLayout> m_pipeline_layout;

    // Random sampling resources (for stochastic shadow rays)
    DualRandomSampleBuffer m_hemisphere_samples;
    std::unique_ptr<NoiseTexture> m_noise_texture;
    std::shared_ptr<const Sampler> m_noise_sampler;

    // Per-frame state
    const BufferBase* m_uniform_buffer = nullptr;
    SkyParameters m_sky_params;
    glm::vec3 m_camera_pos{0.f};
    uint32_t m_frame_count = 0;
};

} // namespace vw
```

**Key differences from the example version:**
- Not templated
- Inherits `RenderPass` (not `Subpass<DirectLightPassSlot>`)
- No `DirectLightPassSlot` enum — uses `Slot::Albedo`, `Slot::Normal`, etc.
- No `DirectLightPassOutput` struct — outputs accessed via `result_images()` and `Slot` keys
- Input: `Slot::Depth` (from ZPass, via `get_input()`)
- Outputs: 7 slots (`Albedo`, `Normal`, `Tangent`, `Bitangent`, `Position`, `DirectLight`, `IndirectRay`)
- UBO via `BufferBase` setter
- Scene and material manager provided at construction (not per-execute)
- `MeshRenderer` is owned (same as example — routes draw calls through per-material pipelines)

### 4.3 Create `DirectLightPass.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/DirectLightPass.cpp`

Port the logic from `examples/Advanced/DirectLightPass.h`:

**Constructor:**
1. Create descriptor set layout (5 bindings: UBO, random samples SSBO, noise texture, sky params UBO, TLAS)
2. Create hemisphere sample buffer via `create_hemisphere_samples_buffer()`
3. Create noise texture
4. Create noise sampler
5. For each material handler in `material_manager`:
   - Generate fragment shader by combining `gbuffer_base.glsl` + handler's `brdf_path()`
   - Compile vertex shader (`GBuffer/gbuffer.vert`)
   - Create graphics pipeline with 7 color attachments + depth attachment
   - Register pipeline in `m_mesh_renderer` for the handler's `MaterialTypeTag`
6. Create pipeline layout (descriptor set layout + material manager's texture layout + push constant range)

**`input_slots()`:** Returns `{Slot::Depth}`
**`output_slots()`:** Returns `{Slot::Albedo, Slot::Normal, Slot::Tangent, Slot::Bitangent, Slot::Position, Slot::DirectLight, Slot::IndirectRay}`

**`execute()`:**
1. Assert `m_uniform_buffer` and `m_ray_traced_scene` are set
2. Get depth view from `get_input(Slot::Depth).view`
3. Allocate 7 output images via `get_or_create_image()` for each slot
4. Set up 7 `vk::RenderingAttachmentInfo` (clear values, store ops) + depth attachment (load from ZPass, read-only)
5. Begin dynamic rendering with all 8 attachments
6. Set viewport and scissor
7. Bind per-frame descriptor set (UBO, samples, noise, sky, TLAS)
8. For each mesh instance in scene:
   - Push model matrix + frame_count + camera_pos via push constants
   - `m_mesh_renderer.draw_mesh(cmd, mesh, transform)`
9. End rendering

### 4.4 Update CMakeLists.txt

**Header:** Add `DirectLightPass.h` to `VulkanWrapper/include/VulkanWrapper/RenderPass/CMakeLists.txt` (PUBLIC)
**Source:** Add `DirectLightPass.cpp` to `VulkanWrapper/src/VulkanWrapper/RenderPass/CMakeLists.txt` (PRIVATE)

### 4.5 Add module partition

**New partition:** `vw.renderpass:direct_light_pass`

Imports needed: `vw.renderpass:render_pass`, `vw.renderpass:slot`, `vw.renderpass:sky_parameters`, `vw.vulkan`, `vw.memory`, `vw.pipeline`, `vw.model`, `vw.raytracing`, `vw.random`, `vw.descriptors`

Update the RenderPass aggregate to re-export this partition.

### 4.6 Create DirectLightPass tests

**File:** `VulkanWrapper/tests/RenderPass/DirectLightPassTests.cpp`

Register in `VulkanWrapper/tests/CMakeLists.txt`.

---

## CMake Registration

| File | Target | Type |
|------|--------|------|
| `include/.../RenderPass/DirectLightPass.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `src/.../RenderPass/DirectLightPass.cpp` | VulkanWrapperCoreLibrary | PRIVATE |

Shaders: `VulkanWrapper/Shaders/GBuffer/gbuffer.vert`, `VulkanWrapper/Shaders/GBuffer/gbuffer_base.glsl` (runtime-compiled)

---

## Dependencies

- **Requires:** Task 1 (RenderPass base), Task 3 (ZPass — provides `Slot::Depth` output)
- **Internal:** `RenderPass`, `MeshRenderer`, `Pipeline`, `PipelineLayout`, `DescriptorSetLayout`, `ShaderCompiler`, `BufferBase`, `Model::Scene`, `Model::Mesh`, `Model::Material::BindlessMaterialManager`, `Model::Material::IMaterialTypeHandler`, `rt::RayTracedScene`, `RandomSamplingBuffer`, `NoiseTexture`, `Sampler`, `SkyParameters`, `Barrier::ResourceTracker`
- **Blocked by:** Task 1, Task 3 (for end-to-end slot wiring, though can be developed independently)
- **Blocks:** Task 5 (AmbientOcclusionPass needs G-Buffer slots), Task 7 (Advanced example)

---

## Test Plan

**File:** `VulkanWrapper/tests/RenderPass/DirectLightPassTests.cpp`

Uses `get_ray_tracing_gpu()` with `GTEST_SKIP()` fallback (requires TLAS for shadow ray queries).

| Test Case | Description |
|-----------|-------------|
| `OutputSlots_Contains7Slots` | `output_slots()` returns all 7 G-Buffer slots |
| `InputSlots_ContainsDepth` | `input_slots()` returns `{Slot::Depth}` |
| `Execute_FillsAllGBufferSlots` | After execute, `result_images()` contains entries for all 7 output slots |
| `Execute_OutputFormats` | Each output image has the expected format (albedo=R8G8B8A8, normal=R32G32B32A32, etc.) |
| `Execute_OutputDimensions` | All output images match requested width/height |
| `Execute_DirectLightNonZero` | For a lit scene (sun above horizon), the DirectLight output has non-zero pixels |
| `Execute_AlbedoNonBlack` | Albedo output contains non-black pixels for textured geometry |

---

## Design Considerations

- **Dynamic fragment shader generation**: DirectLightPass generates per-material fragment shaders by combining `gbuffer_base.glsl` with each handler's `brdf_path()`. This happens at construction time. The same pattern is used by `IndirectLightPass` for hit shaders. If this becomes a common pattern, consider extracting a shared utility, but for now keep it pass-specific.

- **Push constant layout**: The example uses two push constant ranges:
  1. Vertex stage: `MeshPushConstants` (model matrix, from `draw_mesh`)
  2. Fragment stage: `frame_count` + `camera_pos`

  These must be carefully laid out to avoid overlap. The library version should use the same layout.

- **MeshRenderer ownership**: `MeshRenderer` is owned by DirectLightPass. It maps `MaterialTypeTag` → `Pipeline`. The pipelines are created in the constructor from the material manager's registered handlers.

- **Scene reference vs ownership**: DirectLightPass stores a pointer to `RayTracedScene` (set at construction). The scene must outlive the pass. This is the same lifetime model as the example.

- **7 color attachments**: Some GPUs limit the maximum color attachments. Vulkan 1.3 guarantees at least 4. The example already uses 7, so the target hardware must support it. `DeviceFinder` should check `maxColorAttachments >= 7`.

- **Shader include paths**: `gbuffer_base.glsl` uses `#include "include/random.glsl"` etc. The `ShaderCompiler` must have `VulkanWrapper/Shaders/` in its include path. This is already configured for existing library shaders.
