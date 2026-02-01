---
sidebar_position: 3
---

# Sky System

VulkanWrapper provides a physically-based atmospheric rendering system.

## Overview

The sky system includes:
- **SkyParameters**: Physical sky configuration
- **SkyPass**: Atmospheric background rendering
- **SunLightPass**: Direct sun lighting with ray-traced shadows
- **IndirectLightPass**: Indirect sky lighting with progressive accumulation

## SkyParameters

Physical parameters for atmospheric scattering:

```cpp
#include <VulkanWrapper/RenderPass/SkyParameters.h>

// Create Earth-Sun system
auto skyParams = SkyParameters::create_earth_sun();

// Customize
skyParams.sun_direction = glm::normalize(glm::vec3(0.5, 0.3, 0.8));
skyParams.sun_intensity = 100000.0f;  // Radiance in cd/m²
```

### SkyParameters Struct

```cpp
struct SkyParameters {
    glm::vec3 sun_direction;      // Normalized sun direction
    float sun_intensity;          // Sun disk radiance (cd/m²)
    float sun_angular_radius;     // Sun disk angular size
    float planet_radius;          // Planet radius (m)
    float atmosphere_height;      // Atmosphere thickness (m)
    glm::vec3 rayleigh_scattering; // Rayleigh coefficients
    float rayleigh_scale_height;  // Rayleigh scale height (m)
    float mie_scattering;         // Mie scattering coefficient
    float mie_scale_height;       // Mie scale height (m)
    float mie_g;                  // Mie phase asymmetry
};
```

### SkyParametersGPU

GPU-compatible struct (96 bytes, matches GLSL):

```cpp
SkyParametersGPU gpuParams = skyParams.to_gpu();
// Use in push constants or uniform buffer
```

### Factory Methods

```cpp
// Earth with sun
auto earth = SkyParameters::create_earth_sun();

// Custom configuration
SkyParameters custom{
    .sun_direction = glm::vec3(0, 1, 0),
    .sun_intensity = 50000.0f,
    .planet_radius = 6371000.0f,
    // ...
};
```

## SkyPass

Renders the atmospheric background:

```cpp
#include <VulkanWrapper/RenderPass/SkyPass.h>

SkyPass skyPass(device, allocator);

// In render loop
auto& skyOutput = skyPass.render(
    cmd,
    width, height,
    frameIndex,
    skyParams,
    camera.view,
    camera.projection
);
```

### Output

- HDR sky radiance image
- Atmospheric scattering computed per-pixel
- Includes Rayleigh and Mie scattering

## SunLightPass

Direct sun lighting with ray-traced shadows:

```cpp
#include <VulkanWrapper/RenderPass/SunLightPass.h>

SunLightPass sunLightPass(device, allocator);

// Requires ray tracing setup
auto& sunLight = sunLightPass.render(
    cmd,
    width, height,
    frameIndex,
    gbufferPosition,
    gbufferNormal,
    tlas,                // Top-level acceleration structure
    skyParams
);
```

### Features

- Direct sun illumination
- Ray-traced hard shadows
- Uses acceleration structure for shadow rays

## IndirectLightPass

Indirect sky lighting with progressive accumulation:

```cpp
#include <VulkanWrapper/RenderPass/IndirectLightPass.h>

IndirectLightPass skyLightPass(device, allocator);

// Progressive rendering
auto& skyLight = skyLightPass.render(
    cmd,
    width, height,
    frameIndex,
    gbufferPosition,
    gbufferNormal,
    tlas,
    skyParams,
    hemispheresamples,  // Random sampling buffer
    noiseTexture,       // Cranley-Patterson rotation
    accumulatedFrames   // For progressive blending
);
```

### Features

- Hemisphere sampling for ambient occlusion
- Progressive accumulation for noise reduction
- Cranley-Patterson rotation for sample distribution
- Uses constant blending for temporal accumulation

## Shader Includes

Common GLSL includes for sky rendering:

### atmosphere_params.glsl

```glsl
struct SkyParameters {
    vec3 sun_direction;
    float sun_intensity;
    // ... matches CPU struct
};
```

### atmosphere_scattering.glsl

```glsl
// Rayleigh phase function
float rayleigh_phase(float cos_theta);

// Mie phase function (Henyey-Greenstein)
float mie_phase(float cos_theta, float g);

// Atmospheric scattering integration
vec3 compute_scattering(
    vec3 ray_origin,
    vec3 ray_direction,
    SkyParameters params
);
```

### sky_radiance.glsl

```glsl
// Compute sky radiance for direction
vec3 compute_sky_radiance(
    SkyParameters params,
    vec3 direction
);
```

## Integration Example

Complete deferred rendering with sky:

```cpp
// 1. G-Buffer pass
auto& gbuffer = gbufferPass.render(cmd, scene, width, height, frame);

// 2. Sky background
auto& sky = skyPass.render(cmd, width, height, frame,
                           skyParams, view, proj);

// 3. Direct sun lighting
auto& sunLight = sunLightPass.render(
    cmd, width, height, frame,
    gbuffer.position, gbuffer.normal,
    tlas, skyParams);

// 4. Indirect sky lighting (progressive)
auto& skyLight = skyLightPass.render(
    cmd, width, height, frame,
    gbuffer.position, gbuffer.normal,
    tlas, skyParams,
    samples, noise, frameCount);

// 5. Composite and tone map
auto& final = toneMappingPass.render(
    cmd, width, height, frame,
    gbuffer.albedo, sky, sunLight, skyLight);
```

## Best Practices

1. **Use radiance values** - not illuminance
2. **Enable ray tracing** for accurate shadows
3. **Use progressive accumulation** for smooth results
4. **Update sun direction** for time-of-day effects
5. **Cache sky parameters** - don't recreate each frame
