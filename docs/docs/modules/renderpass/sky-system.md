---
sidebar_position: 3
title: "Sky & Lighting System"
---

# Sky & Lighting System

VulkanWrapper includes a physically-based sky and lighting system composed of four render passes. Together, they produce realistic atmospheric rendering with direct sun lighting, screen-space ambient occlusion, and ray-traced indirect illumination.

## SkyParameters

`SkyParameters` contains all the physical parameters needed for atmospheric scattering calculations. It models a planet-star system with configurable scattering coefficients, scale heights, and geometric properties.

### Structure

```cpp
struct SkyParameters {
    // Star (sun) parameters
    float star_constant;        // Solar constant (W/m^2 at top of atmosphere)
    glm::vec3 star_direction;   // Direction FROM star TO planet (normalized)
    glm::vec3 star_color;       // Star color (normalized, typically from temp)
    float star_solid_angle;     // Solid angle of star disk (steradians)

    // Atmospheric scattering coefficients at sea level (per meter)
    glm::vec3 rayleigh_coef;    // Rayleigh scattering coefficient
    glm::vec3 mie_coef;         // Mie scattering coefficient
    glm::vec3 ozone_coef;       // Ozone absorption coefficient

    // Scale heights (meters)
    float height_rayleigh;      // Rayleigh scale height
    float height_mie;           // Mie scale height
    float height_ozone;         // Ozone scale height

    // Planet parameters
    float radius_planet;        // Planet radius (meters)
    float radius_atmosphere;    // Atmosphere outer radius (meters)

    // Luminous efficiency (lm/W) to convert W/m^2/sr to cd/m^2
    float luminous_efficiency;  // Typically 93 lm/W for sunlight
};
```

### Factory Method

The easiest way to get started is with the Earth-Sun preset:

```cpp
auto sky_params = vw::SkyParameters::create_earth_sun(45.0f);
```

The `sun_angle_deg` parameter is the angle above the horizon in degrees. `0` puts the sun at the horizon (sunset/sunrise), `90` puts it directly overhead (noon).

### Static Helper Functions

```cpp
// Convert angle in degrees to a direction vector
static glm::vec3 angle_to_direction(float angle_deg);

// Convert star temperature in Kelvin to RGB color
static glm::vec3 temperature_to_color(float temperature_kelvin);

// Convert angular diameter in degrees to solid angle in steradians
static float angular_diameter_to_solid_angle(float angular_diameter_deg);

// Compute radiance from solar constant and solid angle
static float compute_radiance(float solar_constant, float solid_angle);
```

### Instance Methods

```cpp
// Get the direction toward the star (opposite of star_direction)
glm::vec3 direction_to_star() const;

// Compute star disk radiance from star_constant and star_solid_angle
float star_radiance() const;
```

### GPU Conversion

Sky parameters must be converted to a GPU-friendly format before being sent as push constants or uniform buffer data:

```cpp
SkyParametersGPU to_gpu() const;
```

`SkyParametersGPU` packs all fields into `vec4` members for proper GPU alignment (96 bytes total):

```cpp
struct SkyParametersGPU {
    glm::vec4 star_direction_and_constant;   // xyz = direction, w = constant
    glm::vec4 star_color_and_solid_angle;    // xyz = color, w = solid angle
    glm::vec4 rayleigh_and_height_r;         // xyz = coef, w = height
    glm::vec4 mie_and_height_m;              // xyz = coef, w = height
    glm::vec4 ozone_and_height_o;            // xyz = coef, w = height
    glm::vec4 radii_and_efficiency;          // x = planet, y = atmo, z = efficiency
};
static_assert(sizeof(SkyParametersGPU) == 96);
```

## ZPass (Depth Pre-Pass)

The `ZPass` renders scene geometry into a depth-only attachment. It produces no color output -- just a depth buffer that subsequent passes use for depth testing and position reconstruction.

### Constructor

```cpp
ZPass(std::shared_ptr<Device> device,
      std::shared_ptr<Allocator> allocator,
      const std::filesystem::path &shader_dir,
      vk::Format depth_format = vk::Format::eD32Sfloat);
```

### Slots

- **Inputs**: none
- **Outputs**: `Slot::Depth`

### Configuration

```cpp
// Set the uniform buffer containing view/projection matrices
void set_uniform_buffer(const BufferBase &ubo);

// Set the scene containing mesh instances to render
void set_scene(const rt::RayTracedScene &scene);
```

Both must be set before calling `execute()`.

### Usage

```cpp
auto &z_pass = pipeline.add(
    std::make_unique<vw::ZPass>(device, allocator, shader_dir));
z_pass.set_uniform_buffer(camera_ubo);
z_pass.set_scene(ray_traced_scene);
```

## DirectLightPass (G-Buffer + Direct Sun Lighting)

The `DirectLightPass` is the main geometry pass. It fills the G-Buffer with surface attributes and computes direct sun lighting per fragment using ray queries for real-time shadows.

### Constructor

```cpp
DirectLightPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    const rt::RayTracedScene &ray_traced_scene,
    Model::Material::BindlessMaterialManager &material_manager,
    Formats formats = {});
```

The `Formats` type alias refers to `DirectLightPassFormats`, which controls the format of each G-Buffer attachment:

```cpp
struct DirectLightPassFormats {
    vk::Format albedo       = vk::Format::eR8G8B8A8Unorm;
    vk::Format normal       = vk::Format::eR32G32B32A32Sfloat;
    vk::Format tangent      = vk::Format::eR32G32B32A32Sfloat;
    vk::Format bitangent    = vk::Format::eR32G32B32A32Sfloat;
    vk::Format position     = vk::Format::eR32G32B32A32Sfloat;
    vk::Format direct_light = vk::Format::eR16G16B16A16Sfloat;
    vk::Format indirect_ray = vk::Format::eR32G32B32A32Sfloat;
    vk::Format depth        = vk::Format::eD32Sfloat;
};
```

### Slots

- **Inputs**: `Slot::Depth` (from ZPass, used as read-only depth for early-Z)
- **Outputs**: `Slot::Albedo`, `Slot::Normal`, `Slot::Tangent`, `Slot::Bitangent`, `Slot::Position`, `Slot::DirectLight`, `Slot::IndirectRay`

### Configuration

```cpp
void set_uniform_buffer(const BufferBase &ubo);
void set_sky_parameters(const SkyParameters &params);
void set_camera_position(const glm::vec3 &pos);
void set_frame_count(uint32_t count);
```

### Usage

```cpp
auto &direct = pipeline.add(
    std::make_unique<vw::DirectLightPass>(
        device, allocator, shader_dir,
        ray_traced_scene, material_manager));
direct.set_uniform_buffer(camera_ubo);
direct.set_sky_parameters(
    vw::SkyParameters::create_earth_sun(45.0f));
direct.set_camera_position(camera.position());
direct.set_frame_count(frame_count);
```

## AmbientOcclusionPass

The `AmbientOcclusionPass` computes screen-space ambient occlusion using ray queries against the TLAS. It uses **progressive temporal accumulation**: each frame traces one sample per pixel and blends it with the running average using hardware blend constants.

### Constructor

```cpp
AmbientOcclusionPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    vk::AccelerationStructureKHR tlas,
    vk::Format output_format = vk::Format::eR32G32B32A32Sfloat,
    vk::Format depth_format = vk::Format::eD32Sfloat);
```

### Slots

- **Inputs**: `Slot::Depth`, `Slot::Position`, `Slot::Normal`, `Slot::Tangent`, `Slot::Bitangent`
- **Outputs**: `Slot::AmbientOcclusion`

### Configuration

```cpp
// Set the AO sampling radius (world-space units)
void set_ao_radius(float radius);

// Reset progressive accumulation (call on camera move)
void reset_accumulation() override;

// Get the current accumulated frame count
uint32_t get_frame_count() const;
```

### Progressive Accumulation

The pass accumulates AO samples over multiple frames. When the camera is stationary, each successive frame refines the result, reducing noise. When the camera moves, call `reset_accumulation()` to restart from zero.

The blending uses `ColorBlendConfig::constant_blend()`, where the blend constant is set dynamically each frame to `1.0 / (frame_count + 1)`, producing a running average.

### Usage

```cpp
auto &ao = pipeline.add(
    std::make_unique<vw::AmbientOcclusionPass>(
        device, allocator, shader_dir,
        ray_traced_scene.tlas_handle()));
ao.set_ao_radius(200.0f);

// When camera moves:
ao.reset_accumulation();
```

## SkyPass

The `SkyPass` renders the atmospheric sky where the depth buffer indicates the far plane (`depth == 1.0`). It produces an HDR sky radiance image.

### Constructor

```cpp
SkyPass(std::shared_ptr<Device> device,
        std::shared_ptr<Allocator> allocator,
        const std::filesystem::path &shader_dir,
        vk::Format light_format = vk::Format::eR32G32B32A32Sfloat,
        vk::Format depth_format = vk::Format::eD32Sfloat);
```

### Slots

- **Inputs**: `Slot::Depth`
- **Outputs**: `Slot::Sky`

### Configuration

```cpp
void set_sky_parameters(const SkyParameters &params);
void set_inverse_view_proj(const glm::mat4 &inverse_vp);
```

The sky shader uses the inverse view-projection matrix to reconstruct world-space ray directions from screen-space positions.

### Push Constants

The `SkyPass` combines sky parameters and camera data into a single push constant block:

```cpp
struct PushConstants {
    SkyParametersGPU sky;          // 96 bytes
    glm::mat4 inverseViewProj;     // 64 bytes
};
```

### Usage

```cpp
auto &sky = pipeline.add(
    std::make_unique<vw::SkyPass>(device, allocator, shader_dir));
sky.set_sky_parameters(
    vw::SkyParameters::create_earth_sun(sun_angle));
sky.set_inverse_view_proj(
    glm::inverse(projection * view));
```

## IndirectLightPass

The `IndirectLightPass` computes indirect sky lighting using a **ray tracing pipeline** (not a rasterization pipeline). It traces rays from each visible surface point to gather indirect illumination from the sky atmosphere, accounting for occlusion by scene geometry.

This pass extends `RenderPass` directly (not `ScreenSpacePass`) because it dispatches ray tracing work instead of drawing fullscreen quads.

### Constructor

```cpp
IndirectLightPass(
    std::shared_ptr<Device> device,
    std::shared_ptr<Allocator> allocator,
    const std::filesystem::path &shader_dir,
    const rt::as::TopLevelAccelerationStructure &tlas,
    const rt::GeometryReferenceBuffer &geometry_buffer,
    Model::Material::BindlessMaterialManager &material_manager,
    vk::Format output_format = vk::Format::eR32G32B32A32Sfloat);
```

### Slots

- **Inputs**: `Slot::Position`, `Slot::Normal`, `Slot::Albedo`, `Slot::AmbientOcclusion`, `Slot::IndirectRay`
- **Outputs**: `Slot::IndirectLight`

### Configuration

```cpp
void set_sky_parameters(const SkyParameters &params);

// Reset progressive accumulation (call on camera move)
void reset_accumulation() override;

// Get the current accumulated frame count
uint32_t get_frame_count() const;
```

### Progressive Accumulation

Like `AmbientOcclusionPass`, this pass accumulates samples over multiple frames using `imageLoad`/`imageStore`. Each frame adds one sample per pixel, and the running average converges to the correct indirect lighting over time. Call `reset_accumulation()` when the camera or scene changes.

### Push Constants

```cpp
struct IndirectLightPushConstants {
    SkyParametersGPU sky;   // 96 bytes
    uint32_t frame_count;   //  4 bytes
    uint32_t width;         //  4 bytes
    uint32_t height;        //  4 bytes
};  // 108 bytes total (fits within 128-byte push constant limit)
```

### Usage

```cpp
auto &indirect = pipeline.add(
    std::make_unique<vw::IndirectLightPass>(
        device, allocator, shader_dir,
        ray_traced_scene.tlas(),
        ray_traced_scene.geometry_buffer(),
        material_manager));
indirect.set_sky_parameters(sky_params);

// When camera moves:
indirect.reset_accumulation();
```

## Putting It All Together

Here is the complete lighting pipeline from scene setup to final output:

```cpp
auto sky_params = vw::SkyParameters::create_earth_sun(45.0f);

// Each frame:
z_pass.set_uniform_buffer(camera_ubo);
direct.set_uniform_buffer(camera_ubo);
direct.set_sky_parameters(sky_params);
direct.set_camera_position(camera.position());
direct.set_frame_count(frame_count);
sky.set_sky_parameters(sky_params);
sky.set_inverse_view_proj(glm::inverse(projection * view));
indirect.set_sky_parameters(sky_params);

// If camera moved, reset progressive accumulation:
if (camera_moved) {
    pipeline.reset_accumulation();
}

pipeline.execute(cmd, tracker, width, height, frame_index);
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RenderPass/SkyParameters.h` | `SkyParameters`, `SkyParametersGPU` |
| `VulkanWrapper/RenderPass/ZPass.h` | `ZPass` |
| `VulkanWrapper/RenderPass/DirectLightPass.h` | `DirectLightPass`, `DirectLightPassFormats` |
| `VulkanWrapper/RenderPass/AmbientOcclusionPass.h` | `AmbientOcclusionPass` |
| `VulkanWrapper/RenderPass/SkyPass.h` | `SkyPass` |
| `VulkanWrapper/RenderPass/IndirectLightPass.h` | `IndirectLightPass`, `IndirectLightPushConstants` |
