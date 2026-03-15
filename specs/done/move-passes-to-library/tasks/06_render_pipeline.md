# Task 6: RenderPipeline

## Summary

Create the `RenderPipeline` class that replaces `DeferredRenderingManager`. It provides composable multi-pass rendering with automatic dependency validation via typed slots and automatic input wiring between passes.

---

## Implementation Steps

### 6.1 Create `RenderPipeline.h`

**File:** `VulkanWrapper/include/VulkanWrapper/RenderPass/RenderPipeline.h`

```cpp
namespace vw {

class RenderPipeline {
public:
    RenderPipeline() = default;

    // Add a pass. Returns a reference to the added pass for configuration.
    // Order matters — passes execute in insertion order.
    template <std::derived_from<RenderPass> T>
    T& add(std::unique_ptr<T> pass);

    // Validate: all inputs of every pass are satisfied by a preceding
    // pass's outputs. Returns errors if invalid.
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
    };
    ValidationResult validate() const;

    // Execute all passes in order.
    // Wires outputs from preceding passes to inputs of subsequent passes.
    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index);

    // Reset progressive accumulation in all passes that support it.
    void reset_accumulation();

    // Access passes by index
    RenderPass& pass(size_t index);
    const RenderPass& pass(size_t index) const;
    size_t pass_count() const;

private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
};

} // namespace vw
```

### 6.2 Create `RenderPipeline.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/RenderPass/RenderPipeline.cpp`

**`add()`:**
- Store the pass in `m_passes`
- Return a reference (via `static_cast<T&>`)
- The template method should be defined in the header (template)

**`validate()`:**
1. Build a set of "available slots" (initially empty)
2. For each pass in order:
   - For each slot in `pass->input_slots()`:
     - If the slot is NOT in the available set → add error: `"Pass N requires Slot::X but no preceding pass produces it"`
   - Add all `pass->output_slots()` to the available set
3. Return `{errors.empty(), errors}`

**`execute()`:**
1. Maintain a `std::map<Slot, CachedImage> slot_outputs` across the pipeline
2. For each pass in order:
   - Wire inputs: for each slot in `pass->input_slots()`, call `pass->set_input(slot, slot_outputs[slot])`
   - Call `pass->execute(cmd, tracker, width, height, frame_index)`
   - Collect outputs: for each `{slot, cached_image}` in `pass->result_images()`, store in `slot_outputs`
3. After all passes complete, `slot_outputs` contains the final state of all slots

**`reset_accumulation()`:**
- Iterate all passes
- For each, attempt to call `reset_accumulation()` if the pass supports it
- Two approaches:
  1. **Virtual method in RenderPass**: Add `virtual void reset_accumulation() {}` (default no-op) to the base class
  2. **Dynamic cast**: Try `dynamic_cast` to known pass types
- **Recommended**: Add a virtual no-op `reset_accumulation()` to `RenderPass` base. Only passes with progressive state (AmbientOcclusionPass, IndirectLightPass) override it. This is cleaner than dynamic_cast.

**Note**: This means Task 1's `RenderPass` base class should include:
```cpp
virtual void reset_accumulation() {} // no-op by default
```

### 6.3 Update CMakeLists.txt

**Header:** Add `RenderPipeline.h` to `VulkanWrapper/include/VulkanWrapper/RenderPass/CMakeLists.txt` (PUBLIC)
**Source:** Add `RenderPipeline.cpp` to `VulkanWrapper/src/VulkanWrapper/RenderPass/CMakeLists.txt` (PRIVATE)

### 6.4 Add module partition

**New partition:** `vw.renderpass:render_pipeline`

Imports: `vw.renderpass:render_pass`, `vw.renderpass:slot`

Update the RenderPass aggregate to re-export.

### 6.5 Create RenderPipeline tests

**File:** `VulkanWrapper/tests/RenderPass/RenderPipelineTests.cpp`

Register in `VulkanWrapper/tests/CMakeLists.txt`.

---

## CMake Registration

| File | Target | Type |
|------|--------|------|
| `include/.../RenderPass/RenderPipeline.h` | VulkanWrapperCoreLibrary | PUBLIC |
| `src/.../RenderPass/RenderPipeline.cpp` | VulkanWrapperCoreLibrary | PRIVATE |

---

## Dependencies

- **Requires:** Task 1 (RenderPass base with `set_input()`, `result_images()`, slot introspection)
- **Internal:** `RenderPass`, `Slot`, `CachedImage`, `Barrier::ResourceTracker`
- **Blocked by:** Task 1
- **Blocks:** Task 7 (Advanced example needs RenderPipeline to replace DeferredRenderingManager)

---

## Test Plan

**File:** `VulkanWrapper/tests/RenderPass/RenderPipelineTests.cpp`

Uses lightweight mock/stub passes (concrete subclasses of `RenderPass` with fixed input/output slots and trivial execute()) to test pipeline logic without requiring GPU rendering.

| Test Case | Description |
|-----------|-------------|
| `Validate_EmptyPipeline_Valid` | Empty pipeline validates successfully |
| `Validate_SinglePassNoInputs_Valid` | Pass with no inputs validates |
| `Validate_MissingInput_Invalid` | Pass requires `Slot::Depth` but no preceding pass produces it → error |
| `Validate_InputSatisfiedByPredecessor_Valid` | Pass A outputs `Slot::Depth`, Pass B inputs `Slot::Depth` → valid |
| `Validate_InputFromLaterPass_Invalid` | Pass A inputs `Slot::Depth`, Pass B outputs `Slot::Depth` → error (wrong order) |
| `Validate_MultipleErrors_AllReported` | Multiple unsatisfied inputs → all reported |
| `Execute_WiresOutputsToInputs` | After execute, downstream pass received the upstream output via `get_input()` |
| `Execute_AllPassesCalled` | All passes' execute() methods are called in order |
| `Execute_OutputSlotMap_Complete` | After pipeline execute, all output slots from all passes are available |
| `ResetAccumulation_CallsAllPasses` | `reset_accumulation()` calls the method on all passes |
| `Add_ReturnsTypedReference` | `add()` returns a reference of the concrete pass type |

**Mock Pass for testing:**
```cpp
class MockPass : public vw::RenderPass {
public:
    MockPass(std::shared_ptr<Device> device,
             std::shared_ptr<Allocator> allocator,
             std::vector<Slot> inputs,
             std::vector<Slot> outputs)
        : RenderPass(device, allocator),
          m_inputs(std::move(inputs)),
          m_outputs(std::move(outputs)) {}

    std::vector<Slot> input_slots() const override { return m_inputs; }
    std::vector<Slot> output_slots() const override { return m_outputs; }

    void execute(vk::CommandBuffer cmd,
                 Barrier::ResourceTracker& tracker,
                 Width width, Height height,
                 size_t frame_index) override {
        m_executed = true;
        // Create dummy output images for each output slot
        for (auto slot : m_outputs) {
            get_or_create_image(slot, width, height, frame_index,
                                vk::Format::eR8G8B8A8Unorm,
                                vk::ImageUsageFlagBits::eColorAttachment);
        }
    }

    bool was_executed() const { return m_executed; }

private:
    std::vector<Slot> m_inputs;
    std::vector<Slot> m_outputs;
    bool m_executed = false;
};
```

---

## Design Considerations

- **Validation timing**: `validate()` can be called at any time after pass setup. It should also be callable before `execute()`. The pipeline does NOT automatically validate on execute — the user must call `validate()` explicitly (or we add an assert in debug mode).

- **`reset_accumulation()` in base class**: Adding a virtual no-op to `RenderPass` is a minor Task 1 amendment. This is preferable to `dynamic_cast` chains. Document that passes with progressive state should override this method.

- **Error messages**: Validation errors should be human-readable, e.g.:
  ```
  "Pass 2 (DirectLightPass) requires Slot::Depth but no preceding pass produces it"
  ```
  To include pass names, either:
  1. Add `virtual std::string_view name() const` to RenderPass
  2. Use pass index only

  **Recommended**: Add `virtual std::string_view name() const = 0` to RenderPass (or make it optional with a default returning the type name). Each pass returns its class name.

- **Slot overwriting**: If two passes produce the same slot, the later one's output overwrites the earlier one's in the slot map. This is valid (e.g., multiple sky passes) and should not cause a validation error.

- **Thread safety**: Not needed — all passes execute sequentially in a single command buffer recording.

- **Template method `add()`**: Must be in the header since it's templated. The body is small (move unique_ptr, return cast reference).
