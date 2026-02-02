# Rendering Tests

Test visual coherence through relationships, not pixel-perfect comparisons.

## Philosophy

**Bad:** `EXPECT_EQ(pixel.r, 127)`
**Good:** `EXPECT_GT(luminance_lit, luminance_shadowed * 2.0f)`

No golden images. Test physical plausibility:
- Shadows are darker than lit areas
- Sunset is warmer than noon
- Surfaces facing light are brighter

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
float noon_rb = color_noon.r / color_noon.b;
float sunset_rb = color_sunset.r / color_sunset.b;
EXPECT_GT(sunset_rb, noon_rb) << "Sunset should be warmer";
```

## Pixel Readback

```cpp
glm::vec4 read_center_pixel_hdr(std::shared_ptr<const Image> image) {
    // Copy to staging, read half-floats
    const uint16_t* data = ...;
    return glm::vec4(
        glm::unpackHalf1x16(data[idx]),
        glm::unpackHalf1x16(data[idx + 1]),
        glm::unpackHalf1x16(data[idx + 2]),
        glm::unpackHalf1x16(data[idx + 3])
    );
}
```

## Stochastic Rendering

Accumulate 8-16 frames for stable results:
```cpp
for (int i = 0; i < 16; ++i) {
    result = pass->execute(cmd, tracker, params);
    gpu.queue().submit({}, {}, {}).wait();
}
```

## Guidelines

- Image size: 64x64 (fast) to 256x256 (accurate)
- Tolerance: `EXPECT_NEAR(result, expected, 0.02f)`
- Protect division: `(color.b > 0.001f) ? (color.r / color.b) : color.r`
