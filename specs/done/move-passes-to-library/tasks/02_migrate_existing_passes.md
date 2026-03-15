# Task 2: Migrate Existing Library Passes to New Base

## Summary

Migrate the three existing library passes (`SkyPass`, `ToneMappingPass`, `IndirectLightPass`) from the old `Subpass<SlotEnum>` / `ScreenSpacePass<SlotEnum>` templates to the new non-templated `RenderPass` / `ScreenSpacePass` base classes with typed slot declarations.

Each pass gains `input_slots()` / `output_slots()` overrides and uses `Slot` enum values instead of per-pass enums.

---

## Implementation Steps

### 2.1 Migrate SkyPass

**Files:** `VulkanWrapper/include/VulkanWrapper/RenderPass/SkyPass.h`, `VulkanWrapper/src/VulkanWrapper/RenderPass/SkyPass.cpp`

**Changes:**
1. Remove `enum SkyPassSlot { Light };`
2. Change inheritance: `ScreenSpacePass` (non-templated, from Task 1)
3. Add slot introspection overrides:
   ```cpp
   std::vector<Slot> input_slots() const override { return {Slot::Depth}; }
   std::vector<Slot> output_slots() const override { return {Slot::Sky}; }
   ```
4. Update `get_or_create_image()` calls: replace `SkyPassSlot::Light` → `Slot::Sky`
5. Update `execute()` signature to match `RenderPass::execute()`:
   ```cpp
   void execute(vk::CommandBuffer cmd,
                Barrier::ResourceTracker& tracker,
                Width width, Height height,
                size_t frame_index) override;
   ```
6. The depth view was previously passed as a parameter to `execute()`. Now it comes from `get_input(Slot::Depth)`:
   ```cpp
   // Before: execute(..., depth_view, sky_params, inverse_view_proj)
   // After:  execute(cmd, tracker, width, height, frame_index)
   //         depth_view = get_input(Slot::Depth).view
   ```
7. Add setter methods for non-slot data:
   ```cpp
   void set_sky_parameters(const SkyParameters& params);
   void set_inverse_view_proj(const glm::mat4& inverse_vp);
   ```
8. Store `SkyParameters` and `glm::mat4` as member variables, used in `execute()`.

**Module partition:** Update `vw.renderpass:sky_pass` — remove template slot enum import, add `Slot` import.

### 2.2 Migrate ToneMappingPass

**Files:** `VulkanWrapper/include/VulkanWrapper/RenderPass/ToneMappingPass.h`, `VulkanWrapper/src/VulkanWrapper/RenderPass/ToneMappingPass.cpp`

**Changes:**
1. Remove `enum ToneMappingPassSlot {};` (was empty)
2. Change inheritance: `ScreenSpacePass` (non-templated)
3. Add slot introspection overrides:
   ```cpp
   std::vector<Slot> input_slots() const override {
       return {Slot::Sky, Slot::DirectLight, Slot::IndirectLight};
   }
   std::vector<Slot> output_slots() const override {
       return {Slot::ToneMapped};
   }
   ```
4. Update `execute()` to the unified signature. The three input views come from `get_input()`:
   ```cpp
   void execute(vk::CommandBuffer cmd,
                Barrier::ResourceTracker& tracker,
                Width width, Height height,
                size_t frame_index) override;
   ```
   - `sky_view = get_input(Slot::Sky).view`
   - `direct_light_view = get_input(Slot::DirectLight).view`
   - `indirect_light_view` — check if `Slot::IndirectLight` is wired (optional input)
5. The current `execute()` overload that renders to an external view (swapchain) can remain as a separate non-virtual method, or the pass can allocate its own `Slot::ToneMapped` output image.
   - **Decision**: The pass allocates a `Slot::ToneMapped` output via `get_or_create_image()`. The example blits from this to the swapchain.
6. Remove the fallback black image logic for indirect — if `Slot::IndirectLight` is not wired, use a default black image (keep this behavior).
7. Add setter methods for non-slot data:
   ```cpp
   void set_exposure(float exposure);
   void set_tone_mapping_operator(ToneMappingOperator op);
   void set_white_point(float white_point);
   void set_luminance_scale(float scale);
   void set_indirect_intensity(float intensity);
   ```

**Handling optional IndirectLight input:**
- `input_slots()` always lists `Slot::IndirectLight`
- `RenderPipeline` validation treats it as required unless we add optional input support
- **Option A**: Add `virtual std::vector<Slot> optional_input_slots()` to RenderPass
- **Option B**: `ToneMappingPass` has a mode flag (with/without indirect) that changes `input_slots()` return value
- **Recommended**: Option B — simpler, the user configures whether indirect is used at construction

### 2.3 Migrate IndirectLightPass

**Files:** `VulkanWrapper/include/VulkanWrapper/RenderPass/IndirectLightPass.h`, `VulkanWrapper/src/VulkanWrapper/RenderPass/IndirectLightPass.cpp`

**Changes:**
1. Remove `enum IndirectLightPassSlot { Output };`
2. Change inheritance: `RenderPass` (non-templated)
3. Add slot introspection overrides:
   ```cpp
   std::vector<Slot> input_slots() const override {
       return {Slot::Position, Slot::Normal, Slot::Albedo,
               Slot::AmbientOcclusion, Slot::IndirectRay};
   }
   std::vector<Slot> output_slots() const override {
       return {Slot::IndirectLight};
   }
   ```
4. Update `get_or_create_image()`: replace `IndirectLightPassSlot::Output` → `Slot::IndirectLight`
5. Update `execute()` to unified signature. Input G-Buffer views come from `get_input()`:
   ```cpp
   void execute(vk::CommandBuffer cmd,
                Barrier::ResourceTracker& tracker,
                Width width, Height height,
                size_t frame_index) override;
   ```
   - `position_view = get_input(Slot::Position).view`
   - `normal_view = get_input(Slot::Normal).view`
   - `albedo_view = get_input(Slot::Albedo).view`
   - `ao_view = get_input(Slot::AmbientOcclusion).view`
   - `indirect_ray_view = get_input(Slot::IndirectRay).view`
6. Add setter for non-slot data:
   ```cpp
   void set_sky_parameters(const SkyParameters& params);
   ```
7. Keep `reset_accumulation()` and `get_frame_count()` as-is.

### 2.4 Update existing tests

**Files:**
- `VulkanWrapper/tests/RenderPass/SkyPassTests.cpp`
- `VulkanWrapper/tests/RenderPass/ToneMappingPassTests.cpp`
- `VulkanWrapper/tests/RenderPass/IndirectLightPassTests.cpp`
- `VulkanWrapper/tests/RenderPass/IndirectLightPassSunBounceTests.cpp`

**Changes per test file:**
- Remove references to old slot enums (`SkyPassSlot::Light`, etc.)
- Update pass construction (no template parameter)
- Use `set_*()` setters instead of passing data to `execute()`
- Wire inputs via `set_input()` (or through a test helper) before calling `execute()`
- Update assertions to use `result_images()` with `Slot` keys

---

## CMake Registration

No new files — only modifications to existing files. CMake lists updated in Task 1 already removed `Subpass.h`.

---

## Dependencies

- **Requires:** Task 1 (Slot enum, RenderPass base, ScreenSpacePass base)
- **Blocked by:** Task 1
- **Blocks:** Task 7 (Advanced example update — needs all passes on new API)

---

## Test Plan

Existing test files are updated (not new files):

**`SkyPassTests.cpp`:**

| Test Case | Update |
|-----------|--------|
| Existing sky rendering tests | Use setter API: `set_sky_parameters()`, `set_inverse_view_proj()` |
| `input_slots` / `output_slots` | New: verify `{Depth}` input, `{Sky}` output |
| Slot-based input wiring | New: wire `Slot::Depth` via `set_input()`, verify sky renders correctly |

**`ToneMappingPassTests.cpp`:**

| Test Case | Update |
|-----------|--------|
| Existing tone mapping tests | Use setter API for exposure, operator, etc. |
| `input_slots` / `output_slots` | New: verify inputs and `{ToneMapped}` output |
| Optional indirect light | New: test with and without `Slot::IndirectLight` wired |

**`IndirectLightPassTests.cpp` / `IndirectLightPassSunBounceTests.cpp`:**

| Test Case | Update |
|-----------|--------|
| Existing RT tests | Wire G-Buffer inputs via `set_input()` |
| `input_slots` / `output_slots` | New: verify 5 inputs and `{IndirectLight}` output |
| Progressive accumulation | Unchanged — `reset_accumulation()` / `get_frame_count()` still work |

---

## Design Considerations

- **Optional inputs**: `ToneMappingPass` needs `Slot::IndirectLight` to be optional. Cleanest approach: a constructor parameter or setter `set_indirect_enabled(bool)` that controls whether `input_slots()` includes `Slot::IndirectLight`. When disabled, the pass uses the fallback black image (existing behavior).

- **API surface change**: The old `execute()` methods took input views as parameters. The new `execute()` reads them from `get_input()`. This is a breaking change for any code calling these passes directly (outside `RenderPipeline`). The setter-based approach preserves direct usage for tests.

- **SkyPass depth input**: Currently SkyPass::execute() takes `depth_view` as a parameter and uses it as a read-only depth attachment. In the new system, `get_input(Slot::Depth)` provides the depth image/view. The rendering logic is unchanged — only the source of the depth view changes.
