---
sidebar_position: 4
title: "Tone Mapping"
---

# Tone Mapping

The `ToneMappingPass` converts HDR (high dynamic range) radiance values from the lighting passes into LDR (low dynamic range) values suitable for display. It is typically the final pass in the render pipeline before presenting to the swapchain.

## ToneMappingOperator Enum

VulkanWrapper ships with five tone mapping operators:

```cpp
enum class ToneMappingOperator : int32_t {
    ACES             = 0,  // Academy Color Encoding System
    Reinhard         = 1,  // Simple Reinhard: L / (1 + L)
    ReinhardExtended = 2,  // Reinhard with white point control
    Uncharted2       = 3,  // Hable filmic curve
    Neutral          = 4   // Linear + clamp (no tone mapping)
};
```

### Operator Comparison

| Operator | Behavior | Best For |
|----------|----------|----------|
| **ACES** | Industry-standard filmic curve with good highlight rolloff and color preservation | General-purpose rendering, cinematic look |
| **Reinhard** | Simple `L / (1 + L)` mapping. Gentle highlight compression, can look washed out | Scenes without extreme brightness |
| **ReinhardExtended** | Reinhard with configurable white point. Values above white point map to 1.0 | Scenes where you want control over highlight clipping |
| **Uncharted2** | Filmic S-curve from the game Uncharted 2. Strong contrast, deep shadows | Game-style rendering, high-contrast scenes |
| **Neutral** | No tone mapping at all -- values are linearly clamped to [0, 1] | Debugging, viewing raw HDR values |

## ToneMappingPass

### Constructor

```cpp
ToneMappingPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    vk::Format output_format = vk::Format::eB8G8R8A8Srgb,
    bool indirect_enabled = true);
```

| Parameter | Description |
|-----------|-------------|
| `device` | Vulkan device |
| `allocator` | Memory allocator |
| `shader_dir` | Directory containing `fullscreen.vert` and `tonemap.frag` |
| `output_format` | Output image format. Use an sRGB format (e.g., `eB8G8R8A8Srgb`) to get hardware gamma correction for free |
| `indirect_enabled` | Whether `Slot::IndirectLight` is expected as an input. When `false`, indirect light is not sampled |

### Slots

- **Inputs**: `Slot::Sky`, `Slot::DirectLight`, and optionally `Slot::IndirectLight`
- **Outputs**: `Slot::ToneMapped`

When `indirect_enabled` is `false`, only `Slot::Sky` and `Slot::DirectLight` are required as inputs. The pass uses a 1x1 black fallback image in place of indirect light.

### Configuration

The pass exposes several parameters that control the tone mapping behavior:

```cpp
// Choose the tone mapping operator (default: ACES)
void set_operator(ToneMappingOperator op);
ToneMappingOperator get_operator() const;

// Exposure multiplier applied before tone mapping (default: 1.0)
void set_exposure(float exposure);
float get_exposure() const;

// White point for ReinhardExtended (default: 4.0)
// Values above this are mapped to pure white
void set_white_point(float white_point);
float get_white_point() const;

// Scale factor for luminance values (default: 1.0)
void set_luminance_scale(float scale);
float get_luminance_scale() const;

// Intensity multiplier for indirect light contribution (default: 0.0)
void set_indirect_intensity(float intensity);
float get_indirect_intensity() const;

// Enable/disable indirect light sampling at runtime
void set_indirect_enabled(bool enabled);
bool is_indirect_enabled() const;
```

### Push Constants

All tone mapping parameters are sent to the fragment shader as push constants:

```cpp
struct PushConstants {
    float exposure;
    int32_t operator_id;
    float white_point;
    float luminance_scale;
    float indirect_intensity;
};
```

### Pipeline Usage

When used inside a `RenderPipeline`, the pass reads from slots produced by earlier passes:

```cpp
auto &tonemap = pipeline.add(
    std::make_unique<vw::ToneMappingPass>(
        device, allocator, shader_dir,
        vk::Format::eB8G8R8A8Srgb,
        true));  // indirect light enabled

tonemap.set_operator(vw::ToneMappingOperator::ACES);
tonemap.set_exposure(1.0f);
tonemap.set_indirect_intensity(0.5f);
```

The `RenderPipeline` automatically wires `Slot::Sky` from the `SkyPass`, `Slot::DirectLight` from the `DirectLightPass`, and `Slot::IndirectLight` from the `IndirectLightPass` into the tone mapping pass.

### Rendering to External Image (execute_to_view)

In addition to the standard `execute()` method (which renders to an internally-allocated image via `Slot::ToneMapped`), the pass provides `execute_to_view()` for rendering directly to an external image such as a swapchain image:

```cpp
void execute_to_view(
    vk::CommandBuffer cmd,
    Barrier::ResourceTracker &tracker,
    std::shared_ptr<const ImageView> output_view,
    std::shared_ptr<const ImageView> sky_view,
    std::shared_ptr<const ImageView> direct_light_view,
    std::shared_ptr<const ImageView> indirect_view = nullptr,
    float indirect_intensity = 0.0f,
    ToneMappingOperator tone_operator = ToneMappingOperator::ACES,
    float exposure = 1.0f,
    float white_point = 4.0f,
    float luminance_scale = DEFAULT_LUMINANCE_SCALE);
```

This overload bypasses the slot system and takes explicit image views. It is useful when you want to tone-map directly to the swapchain without going through `RenderPipeline`:

```cpp
tone_mapping.execute_to_view(
    cmd, tracker,
    swapchain_image_view,     // output
    sky_pass_output.view,     // sky input
    direct_light_output.view, // direct light input
    indirect_output.view,     // indirect light input
    0.5f,                     // indirect intensity
    vw::ToneMappingOperator::ACES,
    1.0f,                     // exposure
    4.0f);                    // white point
```

## Gamma Correction

The `ToneMappingPass` does **not** apply gamma correction in the shader. Instead, it relies on the output format to handle gamma:

- Use an **sRGB format** (e.g., `vk::Format::eB8G8R8A8Srgb`) for the output, and the hardware will automatically apply the sRGB transfer function (approximately gamma 2.2).
- If you use a linear format (e.g., `vk::Format::eR8G8B8A8Unorm`), the output will be in linear space and may appear too dark on screen.

## Complete Example

```cpp
// Create the tone mapping pass
auto tone_mapping = std::make_unique<vw::ToneMappingPass>(
    device, allocator, shader_dir,
    vk::Format::eB8G8R8A8Srgb,  // sRGB for hardware gamma
    true);                       // indirect light enabled

// Configure
auto &tm = pipeline.add(std::move(tone_mapping));
tm.set_operator(vw::ToneMappingOperator::ACES);
tm.set_exposure(1.2f);
tm.set_white_point(4.0f);
tm.set_luminance_scale(1.0f);
tm.set_indirect_intensity(0.5f);

// Validate and execute
auto result = pipeline.validate();
assert(result.valid);

pipeline.execute(cmd, tracker, width, height, frame_index);

// The Slot::ToneMapped image now contains the final LDR result
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RenderPass/ToneMappingPass.h` | `ToneMappingPass`, `ToneMappingOperator` |
