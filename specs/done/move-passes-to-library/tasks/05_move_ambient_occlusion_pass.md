# Task 5: Move AmbientOcclusionPass to Library

## Summary

Move `AmbientOcclusionPass` from `examples/Advanced/AmbientOcclusionPass.h` (header-only, ~277 lines) into the VulkanWrapper library as `RenderPass/AmbientOcclusionPass.h` + `AmbientOcclusionPass.cpp`. This is a screen-space pass with progressive temporal accumulation via hardware blending. Move the `ambient_occlusion.frag` shader to the library.

---

## Implementation Steps

### 5.1 Move shader

**From:** `examples/Advanced/Shaders/post-process/ambient_occlusion.frag`
**To:** `VulkanWrapper/Shaders/post-process/ambient_occlusion.frag`

The shader includes `include/random.glsl` which is already in `VulkanWrapper/Shaders/include/`. No path changes needed.

### 5.2 Move fullscreen quad shader (if not already moved)

**From:** `examples/Advanced/Shaders/quad.vert`
**To:** Check if this is identical to `VulkanWrapper/Shaders/fullscreen.vert`

The example's `quad.vert` and the library's `fullscreen.vert` both implement a 4-vertex triangle strip fullscreen quad. If they are identical, reuse `fullscreen.vert`. If `quad.vert` has different UV conventions or output layout, move it as `VulkanWrapper/Shaders/quad.vert` or unify them.

**Action:** Compare the two files. If identical, AmbientOcclusionPass should use `fullscreen.vert`. If different, move `quad.vert` alongside.

### 5.3 Create `AmbientOcclusionPass.h`

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/AmbientOcclusionPass.h`

```cpp
namespace vw {

class AmbientOcclusionPass : public ScreenSpacePass {
public:
    AmbientOcclusionPass(
        std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path& shader_dir,
        vk::AccelerationStructureKHR tlas,
        vk::Format output_format = vk::Format::eR32G32B32A32Sfloat,
        vk::Format depth_format = vk::Format::eD32Sfloat);

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    // Progressive accumulation control
    void reset_accumulation();
    uint32_t get_frame_count() const;

    // Configuration
    void set_ao_radius(float radius);

private:
    vk::Format m_output_format;
    vk::Format m_depth_format;
    vk::AccelerationStructureKHR m_tlas;

    std::shared_ptr<const Pipeline> m_pipeline;
    std::shared_ptr<const PipelineLayout> m_pipeline_layout;
    std::shared_ptr<const DescriptorSetLayout> m_descriptor_set_layout;
    std::shared_ptr<const Sampler> m_sampler;

    // Random sampling resources
    DualRandomSampleBuffer m_hemisphere_samples;
    std::unique_ptr<NoiseTexture> m_noise_texture;

    // Progressive state
    uint32_t m_frame_count = 0;
    float m_ao_radius = 200.0f;
};

} // namespace vw
```

**Key differences from the example version:**
- Inherits `ScreenSpacePass` (non-templated, from Task 1)
- No `AOPassSlot` enum — uses `Slot::AmbientOcclusion`
- Inputs via slot system: `Slot::Depth`, `Slot::Position`, `Slot::Normal`, `Slot::Tangent`, `Slot::Bitangent`
- Output: `Slot::AmbientOcclusion`
- `ao_radius` via setter instead of execute() parameter
- TLAS provided at construction (for ray queries in the fragment shader)

### 5.4 Create `AmbientOcclusionPass.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/AmbientOcclusionPass.cpp`

Port the logic from `examples/Advanced/AmbientOcclusionPass.h`:

**Constructor:**
1. Create descriptor set layout (7 bindings: position, normal, tangent, bitangent sampled images + TLAS + hemisphere samples SSBO + noise texture)
2. Create hemisphere sample buffer
3. Create noise texture + sampler
4. Compile shaders (`fullscreen.vert` or `quad.vert` + `post-process/ambient_occlusion.frag`)
5. Create pipeline via `create_screen_space_pipeline()` with:
   - Depth attachment format (read-only, `CompareOp::eGreater`)
   - Color blend: `ColorBlendConfig::constant_blend()` — the blend factor is set dynamically per frame
   - Push constant range for `{aoRadius, sampleIndex}`

**`input_slots()`:** Returns `{Slot::Depth, Slot::Position, Slot::Normal, Slot::Tangent, Slot::Bitangent}`
**`output_slots()`:** Returns `{Slot::AmbientOcclusion}`

**`execute()`:**
1. Get input views from `get_input()` for all 5 input slots
2. Allocate output image via `get_or_create_image(Slot::AmbientOcclusion, width, height, 0, ...)`:
   - **Fixed frame_index=0** for progressive accumulation (single shared buffer)
3. Set up color attachment:
   - **First frame** (`m_frame_count == 0`): `LoadOp::eClear` with clear color `(1,1,1,1)` (no occlusion)
   - **Subsequent frames**: `LoadOp::eLoad` for temporal accumulation
4. Set up depth attachment (read-only, from `Slot::Depth` input)
5. Compute blend factor: `1.0f / (m_frame_count + 1)`
6. Set blend constants via `cmd.setBlendConstants()`
7. Set push constants: `{m_ao_radius, m_frame_count % DUAL_SAMPLE_COUNT}`
8. Update descriptor set with current input views
9. Call `render_fullscreen()` with the pipeline, descriptor set, push constants, and depth attachment
10. Increment `m_frame_count`

**`reset_accumulation()`:** Reset `m_frame_count = 0`
**`get_frame_count()`:** Return `m_frame_count`

### 5.5 Update CMakeLists.txt

**Header:** Add `AmbientOcclusionPass.h` to `VulkanWrapper/include/VulkanWrapper/RenderPass/CMakeLists.txt` (PUBLIC)
**Source:** Add `AmbientOcclusionPass.cpp` to `VulkanWrapper/src/VulkanWrapper/RenderPass/CMakeLists.txt` (PRIVATE)

### 5.6 Add module partition

**New partition:** `vw.renderpass:ambient_occlusion_pass`

Imports: `vw.renderpass:render_pass`, `vw.renderpass:screen_space_pass`, `vw.renderpass:slot`, `vw.vulkan`, `vw.memory`, `vw.pipeline`, `vw.random`, `vw.descriptors`

Update the RenderPass aggregate to re-export.

### 5.7 Create AmbientOcclusionPass tests

**File:** `VulkanWrapper/tests/RenderPass/AmbientOcclusionPassTests.cpp`

Register in `VulkanWrapper/tests/CMakeLists.txt`.

---

## CMake Registration

| File | Target | Type |
|------|--------|------|
| `include/.../RenderPass/AmbientOcclusionPass.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `src/.../RenderPass/AmbientOcclusionPass.cpp` | VulkanWrapperCoreLibrary | PRIVATE |

Shader: `VulkanWrapper/Shaders/post-process/ambient_occlusion.frag` (runtime-compiled)

---

## Dependencies

- **Requires:** Task 1 (RenderPass + ScreenSpacePass base), Task 4 (DirectLightPass provides G-Buffer slot outputs)
- **Internal:** `ScreenSpacePass`, `Pipeline`, `PipelineLayout`, `DescriptorSetLayout`, `ShaderCompiler`, `RandomSamplingBuffer`, `NoiseTexture`, `Sampler`, `ColorBlendConfig`, `Barrier::ResourceTracker`
- **Blocked by:** Task 1 (for compilation). Task 4 is needed for end-to-end slot wiring but not for isolated development.
- **Blocks:** Task 7 (Advanced example)

---

## Test Plan

**File:** `VulkanWrapper/tests/RenderPass/AmbientOcclusionPassTests.cpp`

Uses `get_ray_tracing_gpu()` with `GTEST_SKIP()` (TLAS needed for ray queries in the AO shader).

| Test Case | Description |
|-----------|-------------|
| `OutputSlots_ContainsAO` | `output_slots()` returns `{Slot::AmbientOcclusion}` |
| `InputSlots_Contains5Slots` | `input_slots()` returns `{Depth, Position, Normal, Tangent, Bitangent}` |
| `Execute_ProducesAOImage` | After execute, `result_images()` has `Slot::AmbientOcclusion` entry |
| `Execute_FirstFrame_ClearsToWhite` | First frame output is white (1,1,1,1) or near-white (AO=1 means no occlusion) |
| `Execute_Progressive_FrameCountIncrements` | `get_frame_count()` increments after each `execute()` |
| `Execute_Progressive_BlendConverges` | After N frames, AO output changes (converges toward occluded values) |
| `ResetAccumulation_ResetsFrameCount` | `reset_accumulation()` sets `get_frame_count()` back to 0 |
| `Execute_AfterReset_ClearsAgain` | After reset, next execute clears to white again |

---

## Design Considerations

- **Progressive accumulation**: Uses hardware blending with `ColorBlendConfig::constant_blend()`. The blend factor `1/(N+1)` is set per-frame via `cmd.setBlendConstants()`. This requires the pipeline to be created with constant blend factors enabled.

- **Fixed frame_index=0**: The AO output uses `frame_index=0` regardless of the swapchain frame index. This ensures a single accumulation buffer is shared across all swapchain images. The same pattern is used by `IndirectLightPass`.

- **Depth comparison**: Uses `CompareOp::eGreater` to discard fragments at the far plane (depth=1.0). Only pixels with actual geometry contribute to AO.

- **Noise texture**: Used for per-pixel Cranley-Patterson rotation to decorrelate samples across pixels. Created at construction time. Same pattern as DirectLightPass.

- **TLAS lifetime**: The pass stores a raw `vk::AccelerationStructureKHR` handle. The acceleration structure must remain valid for the lifetime of the pass. If the scene changes (TLAS rebuilt), the pass should be reconstructed or the handle updated.

- **quad.vert vs fullscreen.vert**: The example's `quad.vert` and the library's `fullscreen.vert` should be compared. If they differ (e.g., different UV layout), either unify them or move `quad.vert` alongside. The AO shader expects specific UV coordinates for screen-space sampling.
