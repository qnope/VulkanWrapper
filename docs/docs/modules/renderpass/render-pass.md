---
sidebar_position: 1
title: "RenderPass & RenderPipeline"
---

# RenderPass & RenderPipeline

VulkanWrapper provides a composable render pass system built around two key abstractions: `RenderPass` (the unit of rendering work) and `RenderPipeline` (the orchestrator that wires passes together). This system uses **slot-based image routing** to automatically connect the output of one pass to the input of another, and **lazy image allocation** so GPU images are only created when first needed.

## Slot Enum

Every image flowing between passes is identified by a `Slot`. Slots act as named channels that describe what kind of data an image carries:

```cpp
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
```

Each pass declares which slots it reads (**input slots**) and which slots it writes (**output slots**). The `RenderPipeline` uses this information to wire outputs from earlier passes into the inputs of later passes.

If the built-in slots do not cover your needs, you can define custom slots starting at `UserSlot`:

```cpp
constexpr auto MyCustomSlot = static_cast<vw::Slot>(
    static_cast<int>(vw::Slot::UserSlot) + 1);
```

## CachedImage

`CachedImage` is the structure that passes use to share images. It bundles an `Image` and its `ImageView` together:

```cpp
struct CachedImage {
    std::shared_ptr<const Image> image;
    std::shared_ptr<const ImageView> view;
};
```

You will encounter `CachedImage` when retrieving pass outputs or when implementing custom passes that create internal images.

## RenderPass Base Class

`RenderPass` is the abstract base class for all rendering passes. It handles lazy image allocation and slot-based input/output wiring.

### Constructor

```cpp
RenderPass(std::shared_ptr<Device> device,
           std::shared_ptr<Allocator> allocator);
```

Every pass needs access to the Vulkan `Device` (for creating pipelines, image views, etc.) and the memory `Allocator` (for creating GPU images).

### Pure Virtual Methods

You must override these four methods when creating a custom pass:

```cpp
// Which slots this pass reads from
virtual std::vector<Slot> input_slots() const = 0;

// Which slots this pass writes to
virtual std::vector<Slot> output_slots() const = 0;

// Perform the actual rendering work
virtual void execute(vk::CommandBuffer cmd,
                     Barrier::ResourceTracker &tracker,
                     Width width, Height height,
                     size_t frame_index) = 0;

// Human-readable name for debugging
virtual std::string_view name() const = 0;
```

### Optional Virtual Method

```cpp
// Reset any temporal accumulation state (no-op by default)
virtual void reset_accumulation() {}
```

Override this if your pass accumulates results over multiple frames (e.g., progressive ambient occlusion). Call it when the camera moves or scene changes to restart accumulation.

### Public Methods

```cpp
// Get the output images produced by the last execute() call
std::vector<std::pair<Slot, CachedImage>> result_images() const;

// Wire an input image for a given slot (called by RenderPipeline)
void set_input(Slot slot, CachedImage image);
```

### Protected Methods

These are available to derived passes:

```cpp
// Get or create an image for the given slot and dimensions.
// Images are cached by (slot, width, height, frame_index).
// When dimensions change, old images are automatically freed.
const CachedImage &get_or_create_image(Slot slot, Width width,
                                       Height height,
                                       size_t frame_index,
                                       vk::Format format,
                                       vk::ImageUsageFlags usage);

// Retrieve an input image that was wired by RenderPipeline
const CachedImage &get_input(Slot slot) const;
```

### Lazy Image Allocation

`get_or_create_image()` is the core mechanism for memory management in the pass system. When a pass calls this method:

1. If an image with matching `(slot, width, height, frame_index)` already exists in the cache, it is returned immediately -- no allocation occurs.
2. If no matching image exists, a new one is created, stored in the cache, and returned.
3. If the dimensions have changed (e.g., the window was resized), images with old dimensions are evicted from the cache.

This means passes never need to worry about image lifetime, resizing, or pre-allocation. Images are created exactly when needed and cleaned up automatically.

### Writing a Custom Pass

Here is a minimal example of a custom render pass:

```cpp
class MyCustomPass : public vw::RenderPass {
public:
    MyCustomPass(std::shared_ptr<vw::Device> device,
                 std::shared_ptr<vw::Allocator> allocator)
        : RenderPass(std::move(device), std::move(allocator)) {}

    std::vector<vw::Slot> input_slots() const override {
        return {vw::Slot::Albedo};
    }

    std::vector<vw::Slot> output_slots() const override {
        return {vw::Slot::ToneMapped};
    }

    void execute(vk::CommandBuffer cmd,
                 vw::Barrier::ResourceTracker &tracker,
                 vw::Width width, vw::Height height,
                 size_t frame_index) override {
        // Lazily allocate the output image
        auto &output = get_or_create_image(
            vw::Slot::ToneMapped, width, height, frame_index,
            vk::Format::eB8G8R8A8Srgb,
            vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

        // Read the input from a previous pass
        auto &input = get_input(vw::Slot::Albedo);

        // ... perform rendering using input.image/input.view
        //     and write to output.image/output.view ...
    }

    std::string_view name() const override {
        return "MyCustomPass";
    }
};
```

## RenderPipeline

`RenderPipeline` is the orchestrator. It holds a list of passes, validates their slot dependencies, and executes them in order while automatically wiring outputs to inputs.

### Adding Passes

```cpp
vw::RenderPipeline pipeline;

auto &z_pass = pipeline.add(
    std::make_unique<vw::ZPass>(device, allocator, shader_dir));

auto &direct_light = pipeline.add(
    std::make_unique<vw::DirectLightPass>(
        device, allocator, shader_dir,
        ray_traced_scene, material_manager));

auto &tone_mapping = pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        device, allocator, shader_dir));
```

The `add()` method takes ownership of the pass via `std::unique_ptr` and returns a typed reference so you can configure the pass after adding it. **Order matters** -- passes execute in the order they are added.

```cpp
template <std::derived_from<RenderPass> T>
T &add(std::unique_ptr<T> pass);
```

### Validation

Before executing, you can validate that all slot dependencies are satisfied:

```cpp
auto result = pipeline.validate();
if (!result.valid) {
    for (const auto &error : result.errors) {
        std::cerr << error << "\n";
    }
}
```

`ValidationResult` is a simple struct:

```cpp
struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
};
```

Validation checks that every input slot required by a pass has been produced as an output slot by some earlier pass.

### Execution

```cpp
pipeline.execute(cmd, tracker, width, height, frame_index);
```

When `execute()` runs, the pipeline:

1. Iterates through passes in insertion order.
2. For each pass, wires the output `CachedImage` from all preceding passes into the current pass's input slots (via `set_input()`).
3. Calls the pass's `execute()` method.
4. Collects the pass's output images (via `result_images()`) for subsequent passes.

### Resetting Accumulation

```cpp
pipeline.reset_accumulation();
```

Calls `reset_accumulation()` on every pass in the pipeline. Use this when the camera moves or the scene changes to restart any progressive rendering effects.

### Accessing Individual Passes

```cpp
RenderPass &pass(size_t index);
const RenderPass &pass(size_t index) const;
size_t pass_count() const;
```

## Pipeline Flow Diagram

The built-in passes form a deferred rendering pipeline with ray-traced lighting:

```
ZPass
  outputs: Depth
     |
     v
DirectLightPass
  inputs:  Depth
  outputs: Albedo, Normal, Tangent, Bitangent,
           Position, DirectLight, IndirectRay
     |
     v
AmbientOcclusionPass
  inputs:  Depth, Position, Normal, Tangent, Bitangent
  outputs: AmbientOcclusion
     |
     v
SkyPass
  inputs:  Depth
  outputs: Sky
     |
     v
IndirectLightPass
  inputs:  Position, Normal, Albedo,
           AmbientOcclusion, IndirectRay
  outputs: IndirectLight
     |
     v
ToneMappingPass
  inputs:  Sky, DirectLight, IndirectLight
  outputs: ToneMapped
     |
     v
  Swapchain
```

Each pass reads from slots produced by earlier passes and writes to new slots, forming a directed acyclic graph of image dependencies.

## Complete Example

```cpp
// Build the full deferred rendering pipeline
vw::RenderPipeline pipeline;

auto &z_pass = pipeline.add(
    std::make_unique<vw::ZPass>(device, allocator, shader_dir));
z_pass.set_uniform_buffer(ubo);
z_pass.set_scene(ray_traced_scene);

auto &direct = pipeline.add(
    std::make_unique<vw::DirectLightPass>(
        device, allocator, shader_dir,
        ray_traced_scene, material_manager));
direct.set_uniform_buffer(ubo);
direct.set_sky_parameters(sky_params);
direct.set_camera_position(camera_pos);

auto &ao = pipeline.add(
    std::make_unique<vw::AmbientOcclusionPass>(
        device, allocator, shader_dir,
        ray_traced_scene.tlas_handle()));

auto &sky = pipeline.add(
    std::make_unique<vw::SkyPass>(
        device, allocator, shader_dir));
sky.set_sky_parameters(sky_params);
sky.set_inverse_view_proj(inverse_vp);

// IndirectLightPass uses ray tracing pipeline (not ScreenSpacePass)
auto &indirect = pipeline.add(
    std::make_unique<vw::IndirectLightPass>(
        device, allocator, shader_dir,
        ray_traced_scene.tlas(),
        ray_traced_scene.geometry_buffer(),
        material_manager));
indirect.set_sky_parameters(sky_params);

auto &tonemap = pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        device, allocator, shader_dir));
tonemap.set_operator(vw::ToneMappingOperator::ACES);
tonemap.set_exposure(1.0f);

// Validate before first use
auto result = pipeline.validate();
assert(result.valid);

// In the render loop:
pipeline.execute(cmd, tracker, width, height, frame_index);
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RenderPass/Slot.h` | `Slot` enum |
| `VulkanWrapper/RenderPass/RenderPass.h` | `RenderPass` base class, `CachedImage` |
| `VulkanWrapper/RenderPass/RenderPipeline.h` | `RenderPipeline`, `ValidationResult` |
