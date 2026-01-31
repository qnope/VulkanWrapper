# Rendering Test Guide

This document explains how to write rendering tests that validate visual output through **coherence testing** rather than pixel-perfect comparisons.

## Philosophy: Coherence Over Pixel-Perfection

Rendering tests in VulkanWrapper do **not** use golden image comparisons or pixel-perfect matching. Instead, they validate **physical plausibility** and **relative relationships** between rendered outputs.

### Why Coherence Testing?

1. **Stochastic Rendering**: Ray tracing uses random sampling, producing slightly different results each frame
2. **Driver Variations**: Different GPUs may produce slightly different floating-point results
3. **Physical Meaning**: What matters is that shadows are darker, sunsets are warmer, etc.
4. **Maintainability**: No golden images to update when shaders improve

## Core Principles

### 1. Test Relationships, Not Absolute Values

**Bad**: "This pixel should be RGB(127, 89, 45)"

**Good**: "Shadowed areas should be darker than lit areas"

```cpp
// Good: Testing a relationship
EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
    << "Lit surface should receive at least 2x more light";

// Bad: Testing absolute values
EXPECT_EQ(pixel.r, 127);
```

### 2. Validate Physical Plausibility

Test that rendering follows physical laws:

- **Rayleigh Scattering**: Blue light scatters more at zenith, red dominates at sunset
- **Lambert's Law**: Surfaces facing light sources receive more illumination
- **Shadow Occlusion**: Occluded surfaces receive less direct light
- **Energy Conservation**: Reflected light cannot exceed incident light

### 3. Use Meaningful Comparisons

Compare two scenarios rendered with the same setup but different parameters:

```cpp
// Render at noon
auto color_noon = render_with_sun_angle(70.0f);

// Render at sunset
auto color_sunset = render_with_sun_angle(5.0f);

// Compare chromatic characteristics
float noon_rb_ratio = color_noon.r / color_noon.b;
float sunset_rb_ratio = color_sunset.r / color_sunset.b;

EXPECT_GT(sunset_rb_ratio, noon_rb_ratio)
    << "Sunset should have higher red-to-blue ratio";
```

## Complete Working Example

Here's a full rendering test from start to finish:

```cpp
#include "utils/create_gpu.hpp"
#include <VulkanWrapper/RenderPass/ToneMappingPass.h>
#include <glm/gtc/packing.hpp>
#include <gtest/gtest.h>

namespace vw::tests {

class ToneMappingTest : public ::testing::Test {
protected:
    void SetUp() override {
        gpu = &create_gpu();
        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(gpu->device).build());
    }

    glm::vec4 read_center_pixel(std::shared_ptr<const Image> image) {
        uint32_t w = image->extent2D().width;
        uint32_t h = image->extent2D().height;
        size_t buffer_size = w * h * 4 * sizeof(uint16_t);

        using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
        auto staging = create_buffer<StagingBuffer>(*gpu->allocator, buffer_size);

        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        Transfer transfer;
        transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);
        std::ignore = cmd.end();

        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();

        auto pixels = staging.read_as_vector(0, buffer_size);
        const uint16_t* data = reinterpret_cast<const uint16_t*>(pixels.data());

        size_t idx = ((h / 2) * w + (w / 2)) * 4;
        return glm::vec4(
            glm::unpackHalf1x16(data[idx]),
            glm::unpackHalf1x16(data[idx + 1]),
            glm::unpackHalf1x16(data[idx + 2]),
            glm::unpackHalf1x16(data[idx + 3])
        );
    }

    GPU* gpu = nullptr;
    std::unique_ptr<CommandPool> cmdPool;
};

TEST_F(ToneMappingTest, BrightInputProducesBrightOutput) {
    // Create HDR input with bright value
    auto input = gpu->allocator->create_image_2D(
        Width{64}, Height{64}, false,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled);

    // Fill with bright HDR color (10.0 radiance)
    // ... (use staging buffer + transfer)

    // Create and execute tone mapping pass
    ToneMappingPass pass(gpu->device, gpu->allocator);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    auto result = pass.execute(cmd, tracker, input_view, params);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();

    // Validate: bright input should produce near-white output
    auto color = read_center_pixel(result->image());
    EXPECT_GT(color.r, 0.8f) << "Bright HDR should map to high LDR";
}

} // namespace vw::tests
```

## Test Image Dimensions

**Recommended sizes:**
- **64x64**: Fast tests, sufficient for color/luminance checks
- **128x128**: Better sampling for stochastic tests
- **256x256**: Use sparingly; slower but more accurate

Choose based on:
- Speed vs accuracy tradeoff
- Stochastic methods need more pixels for stable averages
- Simple coherence tests can use small sizes

## Performance Considerations

- **Accumulation frames**: 8-16 frames balances accuracy vs speed
- **Image size**: Smaller is faster; 64x64 often sufficient
- **GPU wait**: `queue().submit().wait()` is blocking; batch operations when possible
- **CI targets**: Keep individual tests under 5 seconds

## Common Rendering Test Patterns

### Shadow Testing

Test that shadows make surfaces darker:

```cpp
TEST_F(SunLightPassTest, ShadowOcclusion_LitVsShadowed) {
    // Render without occluder
    glm::vec4 color_lit = render_without_shadow();

    // Render with occluder blocking sunlight
    glm::vec4 color_shadowed = render_with_shadow();

    float luminance_lit = color_lit.r + color_lit.g + color_lit.b;
    float luminance_shadowed = color_shadowed.r + color_shadowed.g + color_shadowed.b;

    EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
        << "Lit surface should receive at least 2x more light"
        << " (lit=" << luminance_lit << ", shadowed=" << luminance_shadowed << ")";
}
```

### Diffuse Lighting Direction

Test that surfaces facing the light are brighter:

```cpp
TEST_F(SunLightPassTest, DiffuseLighting_FacingSunVsFacingAway) {
    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

    // Render surface facing sun
    auto color_facing = render_with_normal(to_sun);

    // Render surface facing away from sun
    auto color_away = render_with_normal(-to_sun);

    float luminance_facing = color_facing.r + color_facing.g + color_facing.b;
    float luminance_away = color_away.r + color_away.g + color_away.b;

    EXPECT_GT(luminance_facing, luminance_away * 2.0f)
        << "Surface facing sun should receive more light";
}
```

### Atmospheric Scattering (Sunset vs Noon)

Test that sunset produces warmer (redder) light:

```cpp
TEST_F(SunLightPassTest, AtmosphericAttenuation_SunsetIsWarmerThanNoon) {
    // Noon (sun high at 70 degrees)
    auto color_noon = render_with_sun_angle(70.0f);

    // Sunset (sun low at 5 degrees)
    auto color_sunset = render_with_sun_angle(5.0f);

    // Calculate red-to-blue ratios
    float noon_rb = (color_noon.b > 0.001f)
        ? (color_noon.r / color_noon.b)
        : color_noon.r;
    float sunset_rb = (color_sunset.b > 0.001f)
        ? (color_sunset.r / color_sunset.b)
        : color_sunset.r;

    // Sunset should be warmer (higher R/B ratio)
    EXPECT_GT(sunset_rb, noon_rb)
        << "Sunset should have higher red-to-blue ratio"
        << " (noon R/B=" << noon_rb << ", sunset R/B=" << sunset_rb << ")";

    // Noon should have more total light (less attenuation)
    float luminance_noon = color_noon.r + color_noon.g + color_noon.b;
    float luminance_sunset = color_sunset.r + color_sunset.g + color_sunset.b;

    EXPECT_GT(luminance_noon, luminance_sunset)
        << "Noon should have more total luminance";
}
```

### Sky Color (Zenith vs Horizon)

Test Rayleigh scattering produces blue sky at zenith:

```cpp
TEST_F(SkyLightPassTest, ZenithSun_ProducesBlueIndirectLight) {
    // Sun at zenith (90 degrees)
    auto sky_params = SkyParameters::create_earth_sun(90.0f);

    // Render with multiple frames for accumulation
    auto color = render_accumulated(16);  // 16 frames

    // Blue should dominate (Rayleigh scattering)
    EXPECT_GT(color.b, color.r)
        << "Blue should dominate for zenith sun"
        << " (R=" << color.r << ", G=" << color.g << ", B=" << color.b << ")";
}
```

### Chromatic Shift Comparison

Compare zenith (blue sky) vs horizon (warm sunset):

```cpp
TEST_F(SkyLightPassTest, ChromaticShift_ZenithVsHorizon) {
    // Zenith (blue-dominant)
    auto color_zenith = render_with_sun_angle(90.0f);

    // Horizon (warm-dominant)
    auto color_horizon = render_with_sun_angle(5.0f);

    // Calculate ratios
    float zenith_rb = color_zenith.r / std::max(color_zenith.b, 0.001f);
    float horizon_rb = color_horizon.r / std::max(color_horizon.b, 0.001f);

    // Horizon should be warmer
    EXPECT_GT(horizon_rb, zenith_rb)
        << "Horizon should produce warmer light";
}
```

## Helper Patterns

### G-Buffer Setup

Create test G-buffers with uniform values for controlled testing:

```cpp
struct GBuffer {
    std::shared_ptr<const Image> color;
    std::shared_ptr<const ImageView> color_view;
    std::shared_ptr<const Image> position;
    std::shared_ptr<const ImageView> position_view;
    std::shared_ptr<const Image> normal;
    std::shared_ptr<const ImageView> normal_view;
    // ... additional buffers as needed
};

void fill_gbuffer_uniform(GBuffer& gb,
                          glm::vec3 albedo,
                          glm::vec3 position,
                          glm::vec3 normal,
                          float ao) {
    // Fill all pixels with identical values for controlled testing
    // Use staging buffers and Transfer::copyBufferToImage
}
```

### Pixel Readback for HDR Images

Read center pixel from HDR (R16G16B16A16Sfloat) images:

```cpp
glm::vec4 read_center_pixel_hdr(std::shared_ptr<const Image> image) {
    uint32_t width = image->extent2D().width;
    uint32_t height = image->extent2D().height;
    size_t buffer_size = width * height * 4 * sizeof(uint16_t);

    // Create staging buffer and copy image
    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

    Transfer transfer;
    transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);
    // ... submit and wait ...

    // Read and unpack half-floats
    auto pixels = staging.read_as_vector(0, buffer_size);
    const uint16_t* data = reinterpret_cast<const uint16_t*>(pixels.data());

    size_t x = width / 2;
    size_t y = height / 2;
    size_t pixel_idx = (y * width + x) * 4;

    return glm::vec4(
        glm::unpackHalf1x16(data[pixel_idx]),
        glm::unpackHalf1x16(data[pixel_idx + 1]),
        glm::unpackHalf1x16(data[pixel_idx + 2]),
        glm::unpackHalf1x16(data[pixel_idx + 3])
    );
}
```

### Average Color Calculation

For stochastic rendering, average over all pixels:

```cpp
glm::vec4 read_average_color_hdr(std::shared_ptr<const Image> image) {
    // Copy image to staging buffer...

    const float* data = reinterpret_cast<const float*>(pixels.data());

    glm::vec4 sum(0.0f);
    for (size_t i = 0; i < pixel_count; ++i) {
        sum.r += data[i * 4 + 0];
        sum.g += data[i * 4 + 1];
        sum.b += data[i * 4 + 2];
        sum.a += data[i * 4 + 3];
    }

    return sum / static_cast<float>(pixel_count);
}
```

### Accumulation for Stochastic Rendering

Run multiple frames to reduce variance:

```cpp
std::shared_ptr<const ImageView> result;
for (int i = 0; i < 16; ++i) {
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Barrier::ResourceTracker tracker;
    result = pass->execute(cmd, tracker, /* params */);

    std::ignore = cmd.end();
    gpu->queue().enqueue_command_buffer(cmd);
    gpu->queue().submit({}, {}, {}).wait();
}

// Read converged result
auto color = read_average_color_hdr(result->image());
```

### Ray Tracing Scene Setup (TLAS/BLAS)

For shadow and indirect lighting tests, create a simple scene:

```cpp
class ShadowTest : public ::testing::Test {
protected:
    void SetUp() override {
        gpu = get_ray_tracing_gpu();
        if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";

        // Create mesh manager and load geometry
        gpu->mesh_manager.emplace(*gpu->allocator);

        // Load or create test geometry (plane + occluder)
        auto plane_mesh = gpu->mesh_manager->load_mesh("test_plane.obj");
        auto cube_mesh = gpu->mesh_manager->load_mesh("test_cube.obj");

        // Build acceleration structures
        scene = std::make_unique<rt::RayTracedScene>(
            gpu->device, gpu->allocator);

        // Add ground plane
        scene->add_instance(plane_mesh, glm::mat4(1.0f));

        // Add occluder above (for shadow tests)
        glm::mat4 occluder_transform = glm::translate(
            glm::mat4(1.0f), glm::vec3(0, 2, 0));
        scene->add_instance(cube_mesh, occluder_transform);

        // Build TLAS
        auto cmd = cmdPool->allocate(1)[0];
        std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        scene->build(cmd);

        std::ignore = cmd.end();
        gpu->queue().enqueue_command_buffer(cmd);
        gpu->queue().submit({}, {}, {}).wait();
    }

    RayTracingGPU* gpu = nullptr;
    std::unique_ptr<rt::RayTracedScene> scene;
};
```

For simpler tests without geometry, use synthetic G-buffers (see "G-Buffer Setup" above).

## Test Categories

### Direct Lighting Tests

| Test | What to Validate |
|------|------------------|
| Surface facing sun | Should receive light (luminance > 0) |
| Surface facing away | Should receive no/minimal direct light |
| Facing vs away comparison | Facing sun should be 2x+ brighter |

### Shadow Tests

| Test | What to Validate |
|------|------------------|
| No occluder | Full illumination |
| With occluder | Significantly darker |
| Lit vs shadowed | Lit should be 2x+ brighter |

### Atmospheric Tests

| Test | What to Validate |
|------|------------------|
| Noon (sun high) | Higher total luminance |
| Sunset (sun low) | Higher red-to-blue ratio |
| Zenith sky | Blue dominates over red |
| Horizon sky | Warmer colors (higher R/B) |

### Accumulation Tests

| Test | What to Validate |
|------|------------------|
| Frame count | Increments after each execute |
| Reset | Frame count returns to zero |
| Convergence | Later frames have smaller differences |

## Tolerance Guidelines

For stochastic rendering, use appropriate tolerances:

```cpp
// For ratio comparisons (e.g., R/B ratio)
EXPECT_GT(sunset_rb, zenith_rb)  // No tolerance needed for relative comparison

// For brightness comparisons
EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)  // 2x factor

// For tone mapping (CPU reference comparison)
constexpr float tolerance = 0.02f;
EXPECT_NEAR(gpu_result.r, cpu_reference.r, tolerance);

// For stochastic sampling (relaxed threshold)
EXPECT_GT(red_to_blue_ratio, 0.45f);  // Lower threshold due to variance
```

## Common Mistakes to Avoid

1. **Testing Absolute Pixel Values**
   - Don't: `EXPECT_EQ(pixel.r, 127)`
   - Do: `EXPECT_GT(pixel_lit.r, pixel_shadowed.r)`

2. **Ignoring Accumulation for Stochastic Renders**
   - Don't: Read result after 1 frame
   - Do: Accumulate 8-16 frames for stable results

3. **Forgetting Division by Zero Protection**
   ```cpp
   // Protect against division by zero
   float ratio = (color.b > 0.001f)
       ? (color.r / color.b)
       : color.r;
   ```

4. **Using Too Strict Tolerances**
   - Don't: `EXPECT_NEAR(result, expected, 0.0001f)`
   - Do: `EXPECT_NEAR(result, expected, 0.02f)` for rendering tests

5. **Not Waiting for GPU**
   ```cpp
   // Always wait before reading results
   gpu->queue().submit({}, {}, {}).wait();
   auto color = read_center_pixel_hdr(result->image());
   ```

## Summary

Rendering tests validate that the renderer produces **physically plausible** and **coherent** results:

| Physical Phenomenon | Test Approach |
|--------------------|---------------|
| Shadows | Occluded regions darker than lit |
| Diffuse lighting | Surfaces facing light are brighter |
| Rayleigh scattering | Blue sky at zenith, warm at horizon |
| Atmospheric attenuation | Sunset has less total light, warmer color |
| Surface orientation | Upward-facing sees more sky light |

The key insight is: **test relationships, not absolute values**. A shadowed region should be darker than a lit region - the exact pixel values don't matter as long as this relationship holds.
