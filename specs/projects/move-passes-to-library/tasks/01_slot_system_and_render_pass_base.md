# Task 1: Slot System + RenderPass Base + ScreenSpacePass

## Summary

Create the foundational types for the new render pass system: the `Slot` enum, the non-templated `RenderPass` base class (replacing `Subpass<SlotEnum>`), and the non-templated `ScreenSpacePass` (replacing `ScreenSpacePass<SlotEnum>`). Remove the old templated versions.

This task is the prerequisite for all subsequent tasks.

---

## Implementation Steps

### 1.1 Create `Slot.h`

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/Slot.h`

Define the unified slot enum:

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

    // User extension point
    UserSlot = 1024
};

} // namespace vw
```

This is a simple header-only file with no dependencies beyond `<cstdint>`.

### 1.2 Create `RenderPass.h` (replaces `Subpass.h`)

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/RenderPass.h`

Non-templated base class that replaces `Subpass<SlotEnum>`:

```cpp
namespace vw {

struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};

class RenderPass {
public:
    RenderPass(std::shared_ptr<Device> device,
               std::shared_ptr<Allocator> allocator);
    virtual ~RenderPass() = default;

    // Introspection: which slots this pass reads/writes
    virtual std::vector<Slot> input_slots() const = 0;
    virtual std::vector<Slot> output_slots() const = 0;

    // Execute the pass
    virtual void execute(vk::CommandBuffer cmd,
                         Barrier::ResourceTracker& tracker,
                         Width width, Height height,
                         size_t frame_index) = 0;

    // Get output images after execute()
    std::vector<std::pair<Slot, CachedImage>> result_images() const;

protected:
    // Lazy image allocation (same behaviour as old Subpass)
    const CachedImage& get_or_create_image(Slot slot, Width width,
                                           Height height, size_t frame_index,
                                           vk::Format format,
                                           vk::ImageUsageFlags usage);

    // Access an input image wired by RenderPipeline
    const CachedImage& get_input(Slot slot) const;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<Allocator> m_allocator;

private:
    // Input images populated by RenderPipeline before execute()
    void set_input(Slot slot, CachedImage image);
    friend class RenderPipeline;

    // Output image cache keyed by (Slot, width, height, frame_index)
    struct ImageKey {
        Slot slot;
        uint32_t width;
        uint32_t height;
        size_t frame_index;
        auto operator<=>(const ImageKey&) const = default;
    };
    std::map<ImageKey, CachedImage> m_image_cache;

    // Input images from predecessor passes
    std::map<Slot, CachedImage> m_inputs;
};

} // namespace vw
```

**Key design decisions:**
- `CachedImage` moves from `Subpass.h` to `RenderPass.h` (same struct)
- `result_images()` returns slot-tagged pairs so `RenderPipeline` knows which slot each image belongs to — implementations store their output slot mapping
- `get_or_create_image()` uses `Slot` instead of `SlotEnum` — same lazy allocation and auto-eviction logic from old `Subpass`
- `set_input()` / `get_input()` — managed by `RenderPipeline` (friend), passes read inputs via protected `get_input()`
- `ImageKey` uses `Slot` instead of the old template enum, uses `operator<=>` for `std::map` ordering

### 1.3 Create `RenderPass.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/RenderPass.cpp`

Implement:
- Constructor (store device + allocator)
- `get_or_create_image()` — port logic from `Subpass.h`:
  - Check cache for matching `ImageKey`
  - If found, return cached
  - If found with different dimensions, evict all entries for that slot
  - If not found, create via `m_allocator->create_image_2D()` + `ImageViewBuilder`
- `result_images()` — iterate `m_image_cache`, return `{slot, cached_image}` pairs
- `set_input()` / `get_input()` — simple map insert/lookup

### 1.4 Update `ScreenSpacePass.h` (non-templated)

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/ScreenSpacePass.h`

Replace the template with a concrete class inheriting from `RenderPass`:

```cpp
namespace vw {

class ScreenSpacePass : public RenderPass {
public:
    using RenderPass::RenderPass; // inherit constructor

protected:
    std::shared_ptr<const Sampler> create_default_sampler() const;

    void render_fullscreen(
        vk::CommandBuffer cmd,
        vk::Extent2D extent,
        std::span<const vk::RenderingAttachmentInfo> color_attachments,
        const vk::RenderingAttachmentInfo* depth_attachment,
        const Pipeline& pipeline,
        std::span<const vk::DescriptorSet> descriptor_sets = {},
        std::span<const std::byte> push_constant_data = {},
        vk::ShaderStageFlags push_constant_stages = {});
};

// Free function (unchanged)
std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    /* same signature as current */);

} // namespace vw
```

**Changes from current template:**
- Remove template parameter `SlotEnum`
- Inherit from `RenderPass` instead of `Subpass<SlotEnum>`
- Same `render_fullscreen()` and `create_default_sampler()` methods
- `create_screen_space_pipeline()` free function stays in this header

### 1.5 Update `ScreenSpacePass.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/ScreenSpacePass.cpp`

No logic changes needed — `create_screen_space_pipeline()` is already implemented here. Just update includes/imports if needed.

### 1.6 Delete old `Subpass.h`

**File to delete:** `VulkanWrapper/include/VulkanWrapper/RenderPass/Subpass.h`

The `CachedImage` struct and lazy allocation logic are now in `RenderPass.h` / `RenderPass.cpp`.

### 1.7 Update CMakeLists.txt (headers)

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/CMakeLists.txt`

```cmake
target_sources(VulkanWrapperCoreLibrary PUBLIC
    RenderPass.h         # NEW (replaces Subpass.h)
    Slot.h               # NEW
    ScreenSpacePass.h    # UPDATED (non-templated)
    SkyParameters.h
    SkyPass.h
    ToneMappingPass.h
)
```

Remove `Subpass.h` from the list.

### 1.8 Update CMakeLists.txt (sources)

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/CMakeLists.txt`

```cmake
target_sources(VulkanWrapperCoreLibrary PRIVATE
    RenderPass.cpp       # NEW
    IndirectLightPass.cpp
    ScreenSpacePass.cpp
    SkyParameters.cpp
    SkyPass.cpp
    ToneMappingPass.cpp
)
```

### 1.9 Update module partitions

**New partition:** `vw.renderpass:slot` for `Slot.h`
**New partition:** `vw.renderpass:render_pass` for `RenderPass.h` / `RenderPass.cpp`
**Update partition:** `vw.renderpass:screen_space_pass` — remove template, inherit from RenderPass
**Remove partition:** `vw.renderpass:subpass` (replaced by `render_pass`)

Update the RenderPass aggregate `.cppm` file to export the new partitions.

### 1.10 Update existing tests

**File:** `VulkanWrapper/tests/RenderPass/SubpassTests.cpp`

Rename to `RenderPassTests.cpp` (or keep name but update content):
- Test `RenderPass` lazy image allocation via a concrete test subclass
- Test `get_or_create_image()` caching behavior
- Test auto-eviction on dimension change
- Test `result_images()` returns correct slot-tagged pairs

**File:** `VulkanWrapper/tests/RenderPass/ScreenSpacePassTests.cpp`

- Update to use non-templated `ScreenSpacePass`
- Test `render_fullscreen()` and `create_default_sampler()`

**File:** `VulkanWrapper/tests/CMakeLists.txt`

Update test file references if renamed.

---

## CMake Registration

| File | Target | Type |
|------|--------|------|
| `include/.../RenderPass/Slot.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `include/.../RenderPass/RenderPass.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `include/.../RenderPass/ScreenSpacePass.h` | VulkanWrapperCoreLibrary | PUBLIC (updated) |
| `src/.../RenderPass/RenderPass.cpp` | VulkanWrapperCoreLibrary | PRIVATE |

Remove: `include/.../RenderPass/Subpass.h`

---

## Dependencies

- **Internal:** `Image`, `ImageView`, `Allocator`, `Device`, `Pipeline`, `Sampler`, `ResourceTracker` (all existing Tier 2-5)
- **External:** None new
- **Blocked by:** Nothing (this is the first task)
- **Blocks:** All subsequent tasks (2-7)

---

## Test Plan

**File:** `VulkanWrapper/tests/RenderPass/SubpassTests.cpp` (or renamed to `RenderPassBaseTests.cpp`)

| Test Case | Description |
|-----------|-------------|
| `CreateImage_FirstCall_AllocatesNew` | `get_or_create_image()` creates an image on first call |
| `CreateImage_SameParams_ReturnsCached` | Same (slot, width, height, frame_index) returns the same image |
| `CreateImage_DifferentFrameIndex_CreatesSeparate` | Different frame_index creates a new image |
| `CreateImage_DimensionChange_EvictsOld` | Changing width/height evicts old images for that slot |
| `ResultImages_ReturnsSlotTaggedPairs` | `result_images()` returns the correct slot-to-image mapping |
| `GetInput_AfterSetInput_ReturnsImage` | Input wiring via `set_input()` / `get_input()` works |
| `GetInput_MissingSlot_Throws` | `get_input()` on unwired slot throws or asserts |

**File:** `VulkanWrapper/tests/RenderPass/ScreenSpacePassTests.cpp`

| Test Case | Description |
|-----------|-------------|
| `DefaultSampler_LinearClamp` | `create_default_sampler()` returns a valid sampler with linear + clamp |
| `RenderFullscreen_NoDescriptors_NoCrash` | `render_fullscreen()` with minimal args doesn't crash |

---

## Design Considerations

- **`result_images()` output mapping**: Passes must register which `Slot` each cached image corresponds to. Two options:
  1. Store a `std::map<Slot, CachedImage*>` of output slot → cache entry pointers, updated in `get_or_create_image()`
  2. Since `ImageKey` already contains `Slot`, iterate `m_image_cache` and extract unique slots

  Option 2 is simpler — `result_images()` iterates the cache and returns the most recent entry per slot.

- **Thread safety**: Not required — passes execute sequentially within `RenderPipeline::execute()`.

- **`CachedImage` ownership**: `shared_ptr` for both `Image` and `ImageView` ensures downstream passes can hold references safely across frames.
