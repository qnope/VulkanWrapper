# Rendering Test Guide

This guide covers writing rendering tests for the VulkanWrapper project. Rendering tests validate GPU-based visual output through **coherence checks** rather than pixel-perfect comparisons.

## Table of Contents

1. [Coherence Philosophy](#coherence-philosophy)
2. [Why Not Pixel-Perfect?](#why-not-pixel-perfect)
3. [Helper Functions](#helper-functions)
4. [Test Patterns](#test-patterns)
5. [Lighting Coherence Examples](#lighting-coherence-examples)
6. [Sky/Atmospheric Coherence Examples](#skyatmospheric-coherence-examples)
7. [Shadow Coherence Examples](#shadow-coherence-examples)
8. [Tone Mapping Verification](#tone-mapping-verification)
9. [Ray Tracing Tests](#ray-tracing-tests)
10. [Best Practices](#best-practices)

---

## Coherence Philosophy

**The key principle**: Rendering tests should validate **visual coherence** and **physical plausibility**, not exact pixel values.

### What This Means

| Instead of... | Test this... |
|---------------|--------------|
| "Pixel (10,10) should be RGB(128, 64, 32)" | "Shadowed areas should be darker than lit areas" |
| "Sky color must be exactly #87CEEB" | "Sky at zenith should be more blue than red" |
| "Sun disk should be 20 pixels wide" | "Sunset sky should be warmer (more red) than noon sky" |

### Examples of Coherence Checks

```cpp
// GOOD: Coherence check - relative comparison
EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
    << "Lit surface should receive at least 2x more light than shadowed";

// BAD: Exact pixel value check - fragile
EXPECT_EQ(pixel.r, 128);
EXPECT_EQ(pixel.g, 64);
```

---

## Why Not Pixel-Perfect?

### 1. GPU Precision Varies

Different GPU vendors (NVIDIA, AMD, Intel, Apple Silicon) have slight floating-point precision differences:

```cpp
// This may fail on different GPUs:
EXPECT_EQ(result, 0.333333f);

// This is robust:
EXPECT_NEAR(result, 0.333f, 0.01f);
```

### 2. Stochastic Sampling

Ray tracing uses Monte Carlo sampling with random directions. Results have inherent variance:

```cpp
// Accumulate multiple frames to reduce variance
for (int i = 0; i < 16; ++i) {
    result = pass->execute(cmd, ...);
}
// Then check coherence, not exact values
```

### 3. Shader Optimizations

Minor shader changes (precision hints, instruction reordering) shouldn't break tests:

```cpp
// Instead of exact value:
EXPECT_GT(sunset_rb_ratio, noon_rb_ratio)
    << "Sunset should have higher red-to-blue ratio than noon";
```

### 4. Physical Plausibility Matters More

In graphics, **does it look right?** matters more than **is this the exact number?**

---

## Helper Functions

### Pixel Readback with Staging Buffer

```cpp
// In test fixture class:
glm::vec4 read_first_pixel(std::shared_ptr<const Image> image) {
    uint32_t width = image->extent2D().width;
    uint32_t height = image->extent2D().height;
    size_t buffer_size = width * height * 4;

    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Transfer transfer;
    transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto pixels = staging.read_as_vector(0, buffer_size);

    // Convert to normalized float (for R8G8B8A8 format)
    return glm::vec4(
        static_cast<uint8_t>(pixels[0]) / 255.0f,
        static_cast<uint8_t>(pixels[1]) / 255.0f,
        static_cast<uint8_t>(pixels[2]) / 255.0f,
        static_cast<uint8_t>(pixels[3]) / 255.0f);
}
```

### Reading HDR Pixels (R16G16B16A16Sfloat)

```cpp
glm::vec4 read_center_pixel_hdr(std::shared_ptr<const Image> image) {
    uint32_t width = image->extent2D().width;
    uint32_t height = image->extent2D().height;
    size_t buffer_size = width * height * 4 * sizeof(uint16_t);

    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Transfer transfer;
    transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    auto pixels = staging.read_as_vector(0, buffer_size);
    const uint16_t *data = reinterpret_cast<const uint16_t *>(pixels.data());

    size_t x = width / 2;
    size_t y = height / 2;
    size_t pixel_idx = (y * width + x) * 4;

    // Unpack half-float to float
    return glm::vec4(
        glm::unpackHalf1x16(data[pixel_idx]),
        glm::unpackHalf1x16(data[pixel_idx + 1]),
        glm::unpackHalf1x16(data[pixel_idx + 2]),
        glm::unpackHalf1x16(data[pixel_idx + 3]));
}
```

### Average Color Calculation

```cpp
glm::vec4 read_average_color(std::shared_ptr<const Image> image) {
    // ... setup staging buffer ...

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

### Filling Test Images

```cpp
void fill_hdr_image(std::shared_ptr<const Image> image, glm::vec4 color) {
    uint32_t width = image->extent2D().width;
    uint32_t height = image->extent2D().height;
    size_t pixel_count = width * height;
    size_t buffer_size = pixel_count * 4 * sizeof(uint16_t);

    using StagingBuffer = Buffer<std::byte, true, StagingBufferUsage>;
    auto staging = create_buffer<StagingBuffer>(*allocator, buffer_size);

    std::vector<uint16_t> pixels(pixel_count * 4);
    for (size_t i = 0; i < pixel_count; ++i) {
        pixels[i * 4 + 0] = glm::packHalf1x16(color.r);
        pixels[i * 4 + 1] = glm::packHalf1x16(color.g);
        pixels[i * 4 + 2] = glm::packHalf1x16(color.b);
        pixels[i * 4 + 3] = glm::packHalf1x16(color.a);
    }
    staging.write(std::span<const std::byte>(
        reinterpret_cast<const std::byte *>(pixels.data()), buffer_size), 0);

    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo()
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    Transfer transfer;
    transfer.copyBufferToImage(cmd, staging.handle(), image, 0);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();
}
```

---

## Test Patterns

### Basic Rendering Test Structure

```cpp
class MyRenderPassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();

        cmdPool = std::make_unique<CommandPool>(
            CommandPoolBuilder(device).build());
    }

    // Helper methods...

    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    Queue *queue;
    std::unique_ptr<CommandPool> cmdPool;
};

TEST_F(MyRenderPassTest, SomeCoherenceCheck) {
    // 1. Create resources
    auto output_image = create_test_image(...);

    // 2. Execute pass
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(...);

    pass->execute(cmd, ...);

    std::ignore = cmd.end();
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // 3. Read back and verify coherence
    auto result = read_pixel(output_image);
    EXPECT_GT(result.luminance, expected_minimum);
}
```

---

## Lighting Coherence Examples

### Surface Facing Sun vs Away

**Principle**: A surface facing the sun should receive more light than one facing away.

```cpp
TEST_F(SunLightPassTest, DiffuseLighting_FacingSunVsFacingAway) {
    auto sky_params = SkyParameters::create_earth_sun(45.0f);
    glm::vec3 to_sun = glm::normalize(-sky_params.star_direction);

    // Render surface facing sun
    glm::vec4 color_facing_sun;
    {
        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0), to_sun, 1.0f, 0.5f);
        // Execute pass...
        color_facing_sun = read_center_pixel_light(gb.light);
    }

    // Render surface facing away
    glm::vec4 color_facing_away;
    {
        auto gb = create_gbuffer(width, height);
        fill_gbuffer_uniform(gb, glm::vec3(1.0f), glm::vec3(0), -to_sun, 1.0f, 0.5f);
        // Execute pass...
        color_facing_away = read_center_pixel_light(gb.light);
    }

    float luminance_facing = color_facing_sun.r + color_facing_sun.g + color_facing_sun.b;
    float luminance_away = color_facing_away.r + color_facing_away.g + color_facing_away.b;

    // COHERENCE CHECK: Surface facing sun should receive at least 2x more light
    EXPECT_GT(luminance_facing, luminance_away * 2.0f)
        << "Surface facing sun should receive at least 2x more light"
        << " (facing=" << luminance_facing << ", away=" << luminance_away << ")";
}
```

### Surface Orientation (Up vs Down)

**Principle**: A surface facing up sees the sky; one facing down does not.

```cpp
TEST_F(SkyLightPassTest, SurfaceFacingDown_ReceivesLessLight) {
    // Surface facing up
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    // Execute...
    auto color_up = read_average_color_hdr(result->image());

    // Surface facing down
    fill_gbuffer_uniform(gb, glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
    // Execute...
    auto color_down = read_average_color_hdr(result->image());

    float luminance_up = color_up.r + color_up.g + color_up.b;
    float luminance_down = color_down.r + color_down.g + color_down.b;

    // COHERENCE CHECK: Up-facing surface receives more sky light
    EXPECT_GT(luminance_up, luminance_down * 2.0f)
        << "Surface facing up should receive at least 2x more sky light";
}
```

---

## Sky/Atmospheric Coherence Examples

### Zenith vs Horizon Sky Color

**Principle**: When the sun is at zenith (directly overhead), the sky is predominantly blue due to Rayleigh scattering. At sunset, the sky has warm colors (orange/red).

```cpp
TEST_F(SkyLightPassTest, ZenithSun_ProducesBlueIndirectLight) {
    // Sun at zenith (90 degrees above horizon)
    auto sky_params = SkyParameters::create_earth_sun(90.0f);

    // Execute pass multiple frames for stable accumulation
    for (int i = 0; i < 16; ++i) {
        result = pass->execute(cmd, ...);
    }

    auto color = read_average_color_hdr(result->image());

    // COHERENCE CHECK: Blue should dominate for zenith sun
    EXPECT_GT(color.b, color.r)
        << "Blue should dominate for zenith sun (Rayleigh scattering)"
        << " (R=" << color.r << ", G=" << color.g << ", B=" << color.b << ")";
}
```

### Sunset is Warmer Than Noon

**Principle**: At sunset, light travels through more atmosphere, causing more blue scattering and leaving warmer (red/orange) light.

```cpp
TEST_F(SkyLightPassTest, ChromaticShift_ZenithVsHorizon) {
    // Zenith sun (90 degrees)
    auto color_zenith = render_with_sun_angle(90.0f);

    // Horizon sun (5 degrees - sunset)
    auto color_horizon = render_with_sun_angle(5.0f);

    // Calculate red-to-blue ratios
    float zenith_rb = (color_zenith.b > 0.001f)
        ? (color_zenith.r / color_zenith.b) : 0.0f;
    float horizon_rb = (color_horizon.b > 0.001f)
        ? (color_horizon.r / color_horizon.b) : 0.0f;

    // COHERENCE CHECK: Horizon should have higher red-to-blue ratio
    EXPECT_GT(horizon_rb, zenith_rb)
        << "Horizon sun should produce warmer (higher R/B) indirect light"
        << " (zenith R/B=" << zenith_rb << ", horizon R/B=" << horizon_rb << ")";
}
```

### Noon is Brighter Than Sunset

**Principle**: More atmospheric attenuation at sunset means less total light.

```cpp
TEST_F(SunLightPassTest, AtmosphericAttenuation_SunsetIsWarmerThanNoon) {
    // Noon (sun high, 70 degrees)
    auto color_noon = render_with_sun_angle(70.0f);

    // Sunset (sun low, 5 degrees)
    auto color_sunset = render_with_sun_angle(5.0f);

    float luminance_noon = color_noon.r + color_noon.g + color_noon.b;
    float luminance_sunset = color_sunset.r + color_sunset.g + color_sunset.b;

    // COHERENCE CHECK: Noon should have higher total luminance
    EXPECT_GT(luminance_noon, luminance_sunset)
        << "Noon should have higher total luminance than sunset"
        << " (noon=" << luminance_noon << ", sunset=" << luminance_sunset << ")";
}
```

---

## Shadow Coherence Examples

### Lit vs Shadowed Surfaces

**Principle**: A shadowed surface receives significantly less direct light than a lit surface.

```cpp
TEST_F(SunLightPassTest, ShadowOcclusion_LitVsShadowed) {
    // First: render unshadowed (occluder far away)
    glm::vec4 color_lit;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        // Place occluder far away, not blocking light
        std::ignore = scene.add_instance(plane,
            glm::translate(glm::mat4(1.0f), glm::vec3(0, -1000, 0)));
        scene.build();

        // Execute pass...
        color_lit = read_center_pixel_light(gb.light);
    }

    // Second: render shadowed (occluder blocks light)
    glm::vec4 color_shadowed;
    {
        rt::RayTracedScene scene(gpu->device, gpu->allocator);
        // Large occluder between surface and sun
        glm::mat4 occluder = glm::translate(glm::mat4(1.0f), to_sun * 50.0f);
        occluder = glm::scale(occluder, glm::vec3(100.0f));
        std::ignore = scene.add_instance(plane, occluder);
        scene.build();

        // Execute pass...
        color_shadowed = read_center_pixel_light(gb.light);
    }

    float luminance_lit = color_lit.r + color_lit.g + color_lit.b;
    float luminance_shadowed = color_shadowed.r + color_shadowed.g + color_shadowed.b;

    // COHERENCE CHECK: Lit surface should be significantly brighter
    EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)
        << "Lit surface should receive at least 2x more light than shadowed"
        << " (lit=" << luminance_lit << ", shadowed=" << luminance_shadowed << ")";
}
```

---

## Tone Mapping Verification

For tone mapping, we CAN use CPU reference implementations because the math is deterministic:

```cpp
// CPU-side reference
glm::vec3 tone_map_aces_cpu(glm::vec3 x) {
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

TEST_F(ToneMappingPassTest, Verify_ACESMatchesCPU) {
    glm::vec3 hdr_input(2.0f, 2.0f, 2.0f);
    fill_hdr_image(hdr_image, glm::vec4(hdr_input, 1.0f));

    // Execute GPU tone mapping...

    auto result = read_first_pixel(output_image);
    glm::vec3 expected = tone_map_aces_cpu(hdr_input);

    // Use tolerance for GPU precision differences
    constexpr float tolerance = 0.03f;
    EXPECT_NEAR(result.r, expected.r, tolerance);
    EXPECT_NEAR(result.g, expected.g, tolerance);
    EXPECT_NEAR(result.b, expected.b, tolerance);
}
```

---

## Ray Tracing Tests

### RT GPU Initialization

Ray tracing tests need special GPU features:

```cpp
struct RayTracingGPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;
    std::optional<Model::MeshManager> mesh_manager;

    Queue &queue() { return device->graphicsQueue(); }
};

RayTracingGPU *create_ray_tracing_gpu() {
    try {
        auto instance = InstanceBuilder()
            .setDebug()
            .setApiVersion(ApiVersion::e13)
            .build();

        auto device = instance->findGpu()
            .with_queue(vk::QueueFlagBits::eGraphics)
            .with_synchronization_2()
            .with_dynamic_rendering()
            .with_ray_tracing()           // RT extension
            .with_descriptor_indexing()
            .build();

        auto allocator = AllocatorBuilder(instance, device).build();

        return new RayTracingGPU{...};
    } catch (...) {
        return nullptr;  // RT not available
    }
}
```

### Graceful Skip When RT Unavailable

```cpp
TEST_F(SunLightPassTest, BasicRayTracing) {
    if (!gpu) {
        GTEST_SKIP() << "Ray tracing not available on this system";
    }
    // Test code...
}
```

### Accumulation Convergence

Monte Carlo ray tracing converges over multiple frames:

```cpp
TEST_F(SkyLightPassTest, AccumulationConverges_VarianceDecreases) {
    std::vector<float> differences;
    glm::vec4 prev_color(0.0f);

    for (int frame = 0; frame < 20; ++frame) {
        // Execute pass...
        auto color = read_average_color_hdr(result->image());

        if (frame > 0) {
            float diff = glm::length(color - prev_color);
            differences.push_back(diff);
        }
        prev_color = color;
    }

    // Later differences should be smaller (convergence)
    float early_avg = average(differences.begin(), differences.begin() + 5);
    float late_avg = average(differences.end() - 5, differences.end());

    EXPECT_LT(late_avg, early_avg)
        << "Accumulation should converge (later differences smaller)";
}
```

---

## Best Practices

### 1. Use Relative Comparisons

```cpp
// GOOD: Relative comparison
EXPECT_GT(lit_luminance, shadowed_luminance * 2.0f);

// BAD: Absolute values
EXPECT_EQ(lit_luminance, 0.75f);
```

### 2. Provide Informative Failure Messages

```cpp
EXPECT_GT(sunset_rb_ratio, noon_rb_ratio)
    << "Sunset should have higher red-to-blue ratio than noon"
    << " (noon R/B=" << noon_rb_ratio
    << ", sunset R/B=" << sunset_rb_ratio << ")";
```

### 3. Accumulate for Stochastic Tests

```cpp
// Run multiple frames to reduce variance
for (int i = 0; i < 16; ++i) {
    result = pass->execute(cmd, ...);
}
```

### 4. Use Tolerances Appropriately

```cpp
// For deterministic operations (tone mapping)
constexpr float tolerance = 0.03f;
EXPECT_NEAR(result, expected, tolerance);

// For stochastic operations (ray tracing)
// Use relative comparisons instead
EXPECT_GT(value_a, value_b * factor);
```

### 5. Test Physical Plausibility

Think about what makes sense physically:
- Shadows are darker than lit areas
- Blue skies have more blue than red
- Sunset has warm colors
- Surfaces facing light receive more light

### 6. Document the Physical Principle

```cpp
// When sun is at zenith (directly overhead), the sky is predominantly
// blue due to Rayleigh scattering. Blue light scatters more in short
// atmospheric paths.
TEST_F(SkyLightPassTest, ZenithSun_ProducesBlueIndirectLight) {
    // ...
}
```

---

## Summary

| Principle | Implementation |
|-----------|----------------|
| Shadowed darker than lit | `EXPECT_GT(lit, shadowed * 2.0f)` |
| Sunset warmer than noon | `EXPECT_GT(sunset_rb_ratio, noon_rb_ratio)` |
| Sky at zenith is blue | `EXPECT_GT(color.b, color.r)` |
| Up-facing sees more sky | `EXPECT_GT(up_luminance, down_luminance)` |
| Convergence over frames | `EXPECT_LT(late_variance, early_variance)` |

---

## See Also

- [UnitTest.md](./UnitTest.md) - Unit testing documentation
- [skill.md](./skill.md) - Testing skill overview
