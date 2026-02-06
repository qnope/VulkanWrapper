# Rendering Tests

Test visual coherence through relationships, not pixel-perfect comparisons. See `VulkanWrapper/tests/RenderPass/` for real examples.

## Philosophy

**Bad:** `EXPECT_EQ(pixel.r, 127)`
**Good:** `EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)`

No golden images. Test physical plausibility:
- Shadows are darker than lit areas
- Sunset is warmer (higher red/blue ratio) than noon
- Surfaces facing light are brighter
- Tone-mapped output is in [0, 1] range

## Common Tests

**Shadow occlusion:**
```cpp
auto color_lit = render_without_shadow();
auto color_shadowed = render_with_shadow();
EXPECT_GT(luminance(color_lit), luminance(color_shadowed) * 2.0f)
    << "Lit should be 2x+ brighter";
```

**Atmospheric scattering:**
```cpp
auto color_noon = render_with_sun_angle(70.0f);
auto color_sunset = render_with_sun_angle(5.0f);
float noon_rb = color_noon.r / std::max(color_noon.b, 0.001f);
float sunset_rb = color_sunset.r / std::max(color_sunset.b, 0.001f);
EXPECT_GT(sunset_rb, noon_rb) << "Sunset should be warmer";
```

**Energy conservation:**
```cpp
EXPECT_LE(luminance(output), luminance(input) + epsilon)
    << "Output should not be brighter than input (energy conservation)";
```

## Pixel Readback

Use `Transfer::copyImageToBuffer` to read GPU image data back to CPU:

```cpp
// 1. Create staging buffer for readback
auto staging = create_buffer<uint16_t, true, StagingBufferUsage>(*gpu.allocator, pixel_count * 4);

// 2. Copy image to buffer (Transfer handles barriers)
vw::Transfer transfer;
transfer.resourceTracker().track(ImageState{image->handle(), range, currentLayout, currentStage, currentAccess});
transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

// 3. Submit and wait
gpu.queue().submit({}, {}, {}).wait();

// 4. Read half-float data
auto data = staging.read_as_vector(0, pixel_count * 4);
glm::vec4 pixel(
    glm::unpackHalf1x16(data[idx]),
    glm::unpackHalf1x16(data[idx + 1]),
    glm::unpackHalf1x16(data[idx + 2]),
    glm::unpackHalf1x16(data[idx + 3])
);
```

## Stochastic Rendering

Accumulate 8-16 frames for stable results (Monte Carlo convergence):
```cpp
for (int i = 0; i < 16; ++i) {
    result = pass->execute(cmd, tracker, params);
    gpu.queue().submit({}, {}, {}).wait();
}
```

## Guidelines

- **Image size:** 64x64 (fast) to 256x256 (accurate)
- **Tolerance:** `EXPECT_NEAR(result, expected, 0.02f)` for normalized values
- **Protect division:** `std::max(color.b, 0.001f)` before dividing
- **Luminance helper:** `float luminance(glm::vec4 c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }`
- **Multiple assertions:** Test several relationships per test case for robustness
- **Descriptive messages:** Always include `<< "explanation"` in `EXPECT_*` for debugging
- **Save debug images:** Use `transfer.saveToFile(cmd, allocator, queue, image, "debug.png")` to visually inspect failures
