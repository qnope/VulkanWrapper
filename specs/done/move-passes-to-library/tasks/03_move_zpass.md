# Task 3: Move ZPass to Library

## Summary

Move `ZPass` from `examples/Advanced/ZPass.h` (header-only, templated on UBOType) into the VulkanWrapper library as a proper `.h` + `.cpp` pair under `RenderPass/`. The UBO is provided via a `BufferBase` reference setter instead of a template parameter. Move the `zpass.vert` shader to the library's shader directory.

---

## Implementation Steps

### 3.1 Move shader `zpass.vert`

**From:** `examples/Advanced/Shaders/GBuffer/zpass.vert`
**To:** `VulkanWrapper/Shaders/GBuffer/zpass.vert`

No content changes — the shader is self-contained. It reads from a UBO (binding 0) and push constants (model matrix). The UBO layout (`proj`, `view`, `inverseViewProj`) is defined in the shader, not the C++ type.

### 3.2 Create `ZPass.h`

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/ZPass.h`

```cpp
namespace vw {

class ZPass : public RenderPass {
public:
    ZPass(std::shared_ptr<Device> device,
          std::shared_ptr<Allocator> allocator,
          const std::filesystem::path& shader_dir,
          vk::Format depth_format = vk::Format::eD32Sfloat);

    std::vector<Slot> input_slots() const override;
    std::vector<Slot> output_slots() const override;

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index) override;

    // Configuration setters (call before execute)
    void set_uniform_buffer(const BufferBase& ubo);
    void set_scene(const rt::RayTracedScene& scene);

private:
    vk::Format m_depth_format;
    std::shared_ptr<const Pipeline> m_pipeline;
    std::shared_ptr<const PipelineLayout> m_pipeline_layout;
    std::shared_ptr<const DescriptorSetLayout> m_descriptor_set_layout;

    const BufferBase* m_uniform_buffer = nullptr;
    const rt::RayTracedScene* m_scene = nullptr;
};

} // namespace vw
```

**Key differences from the example version:**
- Not templated — uses `BufferBase` instead of `Buffer<UBOType, ...>`
- Inherits `RenderPass` (not `Subpass<ZPassSlot>`)
- No slot enum — uses `Slot::Depth`
- `input_slots()` returns `{}` (no slot inputs — scene and UBO are set via setters)
- `output_slots()` returns `{Slot::Depth}`
- Scene provided via setter, not execute() parameter
- Pipeline and descriptor layout created in constructor (shader compiled from `shader_dir / "GBuffer/zpass.vert"`)

### 3.3 Create `ZPass.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/ZPass.cpp`

Port the logic from `examples/Advanced/ZPass.h`:

1. **Constructor**: Compile `zpass.vert` shader, create pipeline layout (UBO descriptor + push constants for model matrix), create depth-only graphics pipeline (no fragment shader, depth test `eLess`, depth write enabled).

2. **`execute()`**:
   - Validate `m_uniform_buffer` and `m_scene` are set (assert or throw)
   - Call `get_or_create_image(Slot::Depth, width, height, frame_index, m_depth_format, eDepthStencilAttachment | eSampled)`
   - Set up `vk::RenderingAttachmentInfo` for depth (clear to 1.0, store)
   - Begin dynamic rendering
   - Set viewport and scissor
   - Bind pipeline
   - Bind UBO descriptor set
   - For each mesh instance in scene: push model matrix, call `mesh.draw_zpass(cmd)`
   - End rendering

3. **`set_uniform_buffer()`** / **`set_scene()`**: Store pointers.

**Note on `draw_zpass()`**: The current ZPass calls `instance.mesh.draw_zpass(cmd)`. This method must exist on `Model::Mesh`. Check that it's part of the library API (it should be — `Mesh` is in `vw::Model`).

### 3.4 Update CMakeLists.txt (headers)

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/CMakeLists.txt`

Add `ZPass.h` to the PUBLIC sources.

### 3.5 Update CMakeLists.txt (sources)

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/CMakeLists.txt`

Add `ZPass.cpp` to the PRIVATE sources.

### 3.6 Add module partition

**New partition:** `vw.renderpass:z_pass`

Create the partition `.cppm` file exporting `vw::ZPass`. Import `vw.renderpass:render_pass` and any needed dependencies (`vw.vulkan`, `vw.memory`, `vw.pipeline`, etc.).

Update the RenderPass aggregate to re-export this partition.

### 3.7 Create ZPass tests

**File:** `VulkanWrapper/tests/RenderPass/ZPassTests.cpp`

Register in `VulkanWrapper/tests/CMakeLists.txt` (add to the `RenderPassTests` executable).

---

## CMake Registration

| File | Target | Type |
|------|--------|------|
| `include/.../RenderPass/ZPass.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `src/.../RenderPass/ZPass.cpp` | VulkanWrapperCoreLibrary | PRIVATE |

Shader: `VulkanWrapper/Shaders/GBuffer/zpass.vert` (no CMake registration — compiled at runtime by `ShaderCompiler`)

---

## Dependencies

- **Requires:** Task 1 (RenderPass base class)
- **Internal:** `RenderPass`, `Device`, `Allocator`, `Pipeline`, `PipelineLayout`, `DescriptorSetLayout`, `ShaderCompiler`, `BufferBase`, `Model::Mesh`, `rt::RayTracedScene`, `Barrier::ResourceTracker`
- **Blocked by:** Task 1
- **Blocks:** Task 4 (DirectLightPass needs Depth slot), Task 7 (Advanced example)

---

## Test Plan

**File:** `VulkanWrapper/tests/RenderPass/ZPassTests.cpp`

Uses `create_gpu()` singleton. ZPass does not require ray tracing hardware (depth prepass is rasterization-only), but needs a scene with geometry.

| Test Case | Description |
|-----------|-------------|
| `OutputSlots_ContainsDepth` | `output_slots()` returns `{Slot::Depth}` |
| `InputSlots_Empty` | `input_slots()` returns `{}` |
| `Execute_ProducesDepthImage` | After execute, `result_images()` contains a `Slot::Depth` entry with correct format (`eD32Sfloat`) |
| `Execute_DepthDimensions` | Output depth image has the requested width and height |
| `Execute_DepthValues_Plausible` | For a simple scene (single triangle), depth values are in [0, 1] and not all 1.0 (clear value) — some geometry was rasterized |
| `Execute_Resize_EvictsOld` | Executing with different dimensions creates a new depth image |

---

## Design Considerations

- **`draw_zpass()` method**: ZPass relies on `Mesh::draw_zpass(cmd)` which binds vertex/index buffers and issues draw commands without binding a fragment shader. This is already part of the `Mesh` API in the library.

- **Descriptor set for UBO**: ZPass binds the UBO via a descriptor set (binding 0). The descriptor set is allocated per-frame or uses a dynamic offset. The existing example uses `vk::DescriptorBufferInfo` with the full buffer range. The library version should do the same.

- **Scene access**: ZPass iterates `scene.instances()` to get mesh + transform pairs. `RayTracedScene::scene()` returns the `Model::Scene` used for rasterization. The setter takes `const rt::RayTracedScene&` to access both the scene graph and the TLAS (though ZPass only uses the scene graph for rasterization).

- **Shader directory**: The constructor takes a `shader_dir` path. This should point to the `VulkanWrapper/Shaders/` directory. The pass compiles `shader_dir / "GBuffer/zpass.vert"`.
