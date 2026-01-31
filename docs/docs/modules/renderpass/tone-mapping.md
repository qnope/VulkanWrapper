---
sidebar_position: 4
---

# Tone Mapping

The `ToneMappingPass` converts HDR rendered images to LDR for display.

## Overview

```cpp
#include <VulkanWrapper/RenderPass/ToneMappingPass.h>

ToneMappingPass toneMappingPass(device, allocator);

auto& output = toneMappingPass.render(
    cmd,
    width, height,
    frameIndex,
    hdrInput,
    ToneMapOperator::ACES
);
```

## ToneMapOperator

Available tone mapping operators:

```cpp
enum class ToneMapOperator {
    None,       // Linear (no tone mapping)
    Reinhard,   // Simple Reinhard
    ACES,       // Academy Color Encoding System
    Uncharted2, // Uncharted 2 filmic
    Neutral     // Neutral tone curve
};
```

### Comparison

| Operator | Description | Use Case |
|----------|-------------|----------|
| None | Linear clamp | Debug, HDR displays |
| Reinhard | Simple curve | General purpose |
| ACES | Film-like curve | Cinematic look |
| Uncharted2 | Filmic response | Games, realistic |
| Neutral | Balanced curve | Neutral rendering |

## ToneMappingPass

### Constructor

```cpp
ToneMappingPass(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator
);
```

### Render Method

```cpp
ImageView& render(
    CommandBuffer& cmd,
    uint32_t width,
    uint32_t height,
    uint32_t frameIndex,
    const ImageView& hdrInput,
    ToneMapOperator op = ToneMapOperator::ACES
);
```

### With Indirect Light

```cpp
ImageView& render(
    CommandBuffer& cmd,
    uint32_t width,
    uint32_t height,
    uint32_t frameIndex,
    const ImageView& hdrInput,
    const ImageView& indirectLight,
    float indirectBlend,
    ToneMapOperator op = ToneMapOperator::ACES
);
```

## Push Constants

The pass uses push constants for parameters:

```cpp
struct ToneMappingParams {
    uint32_t operator_type;
    float exposure;
    float gamma;
    float indirect_blend;
};
```

## Implementation Details

### ACES Tone Mapping

```glsl
vec3 aces_tonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
```

### Reinhard

```glsl
vec3 reinhard_tonemap(vec3 x) {
    return x / (1.0 + x);
}
```

### Uncharted 2

```glsl
vec3 uncharted2_tonemap(vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) /
            (x * (A * x + B) + D * F)) - E / F;
}
```

## Exposure Control

Adjust exposure before tone mapping:

```cpp
struct ToneMappingParams {
    float exposure = 1.0f;  // Exposure multiplier
};

// In shader
vec3 exposed = hdrColor * exposure;
vec3 mapped = tonemap(exposed);
```

## Gamma Correction

Apply gamma after tone mapping:

```cpp
// In shader
vec3 gammaCorrected = pow(mapped, vec3(1.0 / gamma));
```

## Usage Example

```cpp
class Renderer {
public:
    void render(CommandBuffer& cmd, uint32_t frame) {
        // Render HDR scene
        auto& hdr = lightingPass.render(cmd, width, height, frame, ...);

        // Optional: indirect lighting
        auto& indirect = skyLightPass.render(cmd, ...);

        // Tone map to LDR
        auto& ldr = toneMappingPass.render(
            cmd,
            width, height,
            frame,
            hdr,
            indirect,
            0.5f,  // 50% indirect blend
            ToneMapOperator::ACES
        );

        // Present ldr to swapchain
    }
};
```

## Best Practices

1. **Use ACES** for cinematic results
2. **Adjust exposure** based on scene brightness
3. **Apply gamma 2.2** for sRGB displays
4. **Blend indirect light** for ambient contribution
5. **Consider auto-exposure** for dynamic scenes
