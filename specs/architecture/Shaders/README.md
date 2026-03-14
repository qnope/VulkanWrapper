# Shaders

`VulkanWrapper/Shaders/` directory

GLSL shader source files compiled to SPIR-V at runtime by `ShaderCompiler`.

## Library Shaders

| File | Purpose |
|------|---------|
| `fullscreen.vert` | Fullscreen quad from vertex ID (4 vertices, triangle strip). Used by all ScreenSpacePass derivatives. |
| `sky.frag` | Atmospheric sky rendering (Rayleigh + Mie scattering) |
| `tonemap.frag` | HDR to LDR conversion with multiple tone mapping operators |

## Include Directory (`Shaders/include/`)

| File | Purpose |
|------|---------|
| `random.glsl` | GPU random number generation utilities |
| `atmosphere_params.glsl` | Atmosphere parameter struct definitions |
| `atmosphere_scattering.glsl` | Rayleigh/Mie scattering computation |
| `geometry_access.glsl` | Vertex attribute access from GeometryReferenceBuffer in hit shaders |
| `indirect_light_base.glsl` | Base template for per-material indirect light closest hit shaders |
| `sky_radiance.glsl` | Sky radiance computation utilities |
| `sun_lighting_computation.glsl` | Sun direct lighting calculations |

## Material BRDFs (`Shaders/include/Material/`)

| File | Material Handler |
|------|-----------------|
| `brdf_colored.glsl` | `ColoredMaterialHandler` |
| `brdf_textured.glsl` | `TexturedMaterialHandler` |
| `brdf_emissive_textured.glsl` | `EmissiveTexturedMaterialHandler` |

These are combined with `indirect_light_base.glsl` by `ShaderCompiler` to generate per-material closest hit shaders at runtime. Each BRDF snippet defines the material-specific albedo/emissive evaluation.
