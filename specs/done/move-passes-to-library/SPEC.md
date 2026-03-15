# Move Render Passes to Library — API Specification

## 1. Feature Overview

Move the three deferred rendering passes currently living in `examples/Advanced/` (`ZPass`, `DirectLightPass`, `AmbientOcclusionPass`) into the `VulkanWrapper` library under `RenderPass/`. Replace the rigid `DeferredRenderingManager` with a composable `RenderPipeline` that lets users build custom multi-pass pipelines with automatic dependency validation via typed Input/Output slots.

All existing library passes (`SkyPass`, `ToneMappingPass`, `IndirectLightPass`) are also migrated to the new typed-slot system for API consistency.

The Advanced example is updated to import the passes from the library and use the new `RenderPipeline` API, producing the same rendering output.

---

## 2. API Design

### 2.1 Typed Slot System

Replace the current `Subpass<SlotEnum>` template (per-pass enums) with a unified, type-safe slot system where passes declare their inputs and outputs explicitly.

#### Slot Registry

A global enum (or tag-type set) defines all available slot identifiers:

```cpp
namespace vw {

enum class Slot {
    // Geometry
    Depth,

    // G-Buffer
    Albedo,
    Normal,
    Tangent,
    Bitangent,
    Position,
    DirectLight,
    IndirectRay,

    // Post-process
    AmbientOcclusion,
    Sky,
    IndirectLight,

    // Final
    ToneMapped,
    UserSlot = 1024
};

} // namespace vw
```

#### Input / Output Declarations

Each pass declares typed inputs and outputs:

```cpp
namespace vw {

template <Slot S>
struct Input {
    static constexpr Slot slot = S;
    std::shared_ptr<const ImageView> view;
};

template <Slot S>
struct Output {
    static constexpr Slot slot = S;
    std::shared_ptr<const ImageView> view;
};

} // namespace vw
```

#### Base Class: `RenderPass`

Replaces `Subpass<SlotEnum>`. No longer templated — all passes inherit from the same base:

```cpp
namespace vw {

class RenderPass {
public:
    RenderPass(std::shared_ptr<Device> device,
               std::shared_ptr<Allocator> allocator);
    virtual ~RenderPass() = default;

    // Introspection: which slots does this pass read/write?
    virtual std::vector<Slot> input_slots() const = 0;
    virtual std::vector<Slot> output_slots() const = 0;

    // Execute the pass
    virtual void execute(vk::CommandBuffer cmd,
                         Barrier::ResourceTracker& tracker,
                         Width width, Height height,
                         size_t frame_index) = 0;

    std::vector<CachedImage> result_images() const;

protected:
    // Lazy image allocation (same behaviour as current Subpass)
    const CachedImage& get_or_create_image(Slot slot, Width width,
                                           Height height, size_t frame_index,
                                           vk::Format format,
                                           vk::ImageUsageFlags usage);

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;

private:
    // Image cache keyed by (Slot, width, height, frame_index)
    struct ImageKey { /* ... */ };
    std::map<ImageKey, CachedImage> m_image_cache;
};

} // namespace vw
```

### 2.2 Composable RenderPipeline

Replaces `DeferredRenderingManager`:

```cpp
namespace vw {

class RenderPipeline {
public:
    // Add a pass. Order matters — passes execute in insertion order.
    template <std::derived_from<RenderPass> T>
    T& add(std::unique_ptr<T> pass);

    // Validate: all inputs of every pass are satisfied by a preceding
    // pass's outputs. Returns error messages if invalid.
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
    };
    ValidationResult validate() const;

    // Execute all passes in order.
    // before calling execute.
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index);

    // Reset all passes that support progressive accumulation
    void reset_accumulation();
};

} // namespace vw
```

**Usage example (replacing DeferredRenderingManager):**

```cpp
// Setup
vw::RenderPipeline pipeline;
auto &zpass = pipeline.add(std::make_unique<vw::ZPass>(device, allocator, ...));
auto &directLightPass = pipeline.add(std::make_unique<vw::DirectLightPass>(device, allocator, ...));
auto &aoPass = pipeline.add(std::make_unique<vw::AmbientOcclusionPass>(device, allocator, ...));
auto &skyPass = pipeline.add(std::make_unique<vw::SkyPass>(device, allocator, ...));
auto &indirectLightPass = pipeline.add(std::make_unique<vw::IndirectLightPass>(device, allocator, ...));
auto &toneMappingPass = pipeline.add(std::make_unique<vw::ToneMappingPass>(device, allocator, ...));

auto result = pipeline.validate();
assert(result.valid);

// Per-frame
skyPass.set_sky_parameters(sky_params);
zpass.set_uniform_buffer(uniform_buffer);
...
// ...

pipeline.execute(cmd, tracker, width, height, frame_index);

// Read final output : only one image output from tonemapping pass
auto final_view = toneMappingPass.result_images().front();
transfer.blit(cmd, final_view->image(), swapchain_image);
```

### 2.3 Moved Passes

All passes live in `namespace vw` under `VulkanWrapper/include/VulkanWrapper/RenderPass/`.

#### ZPass

- **Inputs:** none (reads `scene` and `uniform_buffer`)
- **Outputs:** `Slot::Depth`
- **Files:** `ZPass.h` + `ZPass.cpp`

#### DirectLightPass

- **Inputs:** `Slot::Depth` (from ZPass)
- **Outputs:** `Slot::Albedo`, `Slot::Normal`, `Slot::Tangent`, `Slot::Bitangent`, `Slot::Position`, `Slot::DirectLight`, `Slot::IndirectRay`
- **Files:** `DirectLightPass.h` + `DirectLightPass.cpp`

#### AmbientOcclusionPass

- **Inputs:** `Slot::Depth`, `Slot::Position`, `Slot::Normal`, `Slot::Tangent`, `Slot::Bitangent`
- **Outputs:** `Slot::AmbientOcclusion`
- **Files:** `AmbientOcclusionPass.h` + `AmbientOcclusionPass.cpp`
- Retains `reset_accumulation()` / `get_frame_count()`.

#### SkyPass (migrated)

- **Inputs:** `Slot::Depth`
- **Outputs:** `Slot::Sky`

#### IndirectLightPass (migrated)

- **Inputs:** `Slot::Position`, `Slot::Normal`, `Slot::Albedo`, `Slot::AmbientOcclusion`, `Slot::IndirectRay`
- **Outputs:** `Slot::IndirectLight`

#### ToneMappingPass (migrated)

- **Inputs:** `Slot::Sky`, `Slot::DirectLight`, `Slot::IndirectLight` (optional)
- **Outputs:** `Slot::ToneMapped`

### 2.4 Data Structures

- `DirectLightPassOutput` is removed (replaced by slot-based output).
- `GBuffer`, `GBufferInformation`, `TonemapInformation` from `RenderPassInformation.h` are no longer needed (slots replace them).
- `UBOData` and `PushConstantData` remain in the example — they are application-specific.

### 2.5 Module Integration

The moved passes join the `vw.renderpass` module:
- New partitions: `vw.renderpass:z_pass`, `vw.renderpass:direct_light_pass`, `vw.renderpass:ambient_occlusion_pass`, `vw.renderpass:render_pipeline`, `vw.renderpass:slot`
- Existing partitions (`subpass`, `screen_space_pass`, `sky_pass`, `tone_mapping_pass`, `indirect_light_pass`, `sky_parameters`) are updated.

---

## 3. Testing and Validation

### 3.1 Unit Tests (GTest)

New test files in `VulkanWrapper/tests/RenderPass/`:

| Test file | Scope |
|-----------|-------|
| `ZPassTests.cpp` | ZPass produces a depth image of correct format/dimensions; depth values are plausible for a simple scene |
| `DirectLightPassTests.cpp` | DirectLightPass fills all 7 G-Buffer slots; output formats match expectations |
| `AmbientOcclusionPassTests.cpp` | AO pass produces non-zero output; progressive accumulation converges (frame_count increments, blend factor decreases) |
| `RenderPipelineTests.cpp` | Validate: missing input → error; valid pipeline → ok; execute runs without crash; slots are populated after execute |

All tests use the existing `create_gpu()` singleton. Ray-tracing–dependent tests (DirectLightPass, AmbientOcclusionPass) use `get_ray_tracing_gpu()` with `GTEST_SKIP()` fallback.

### 3.2 Integration Test

- The Advanced example (`examples/Advanced/`) is updated to use the library passes and `RenderPipeline`.
- Running `Main` must produce a `screenshot.png` visually identical to the current output.

### 3.3 Validation Criteria

- `cmake --build build` succeeds.
- All existing tests pass (`ctest --stop-on-failure`).
- New RenderPass tests pass.
- `Main` example produces correct output.
- No files remain in `examples/Advanced/` that duplicate library code (ZPass.h, DirectLightPass.h, AmbientOcclusionPass.h, DeferredRenderingManager.h are removed or replaced by thin wrappers).
