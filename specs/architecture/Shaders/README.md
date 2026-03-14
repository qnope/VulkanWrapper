# Shaders

`VulkanWrapper/Shaders/` directory

GLSL shader source files compiled to SPIR-V at runtime by `ShaderCompiler`.

## Library Shaders

| File | Purpose |
|------|---------|
| `fullscreen.vert` | Fullscreen quad from vertex ID (4 vertices, triangle strip). Used by all ScreenSpacePass derivatives |
| `sky.frag` | Atmospheric sky rendering (Rayleigh + Mie scattering) |
| `tonemap.frag` | HDR to LDR conversion with multiple tone mapping operators |
| `GBuffer/gbuffer.vert` | G-Buffer geometry pass vertex shader |
| `GBuffer/zpass.vert` | Depth prepass vertex shader |
| `GBuffer/gbuffer_base.glsl` | Base template for per-material G-Buffer fragment shaders (combined with BRDF snippets at runtime) |
| `post-process/ambient_occlusion.frag` | Screen-space ambient occlusion via ray queries |

## Ray Tracing Shaders

| File | Purpose |
|------|---------|
| `indirect_light.rgen` | Ray generation for indirect sky lighting |
| `indirect_light.rmiss` | Miss shader (sky radiance fallback) |

Per-material closest hit shaders are generated dynamically from `indirect_light_base.glsl` + handler `brdf_path()`.

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

| File | `evaluate_brdf` | `emissive_light` | `generate_ray` |
|------|-----------------|-------------------|----------------|
| `brdf_colored.glsl` | `color / pi` | `vec3(0)` | Cosine hemisphere |
| `brdf_textured.glsl` | `texture_color / pi` | `vec3(0)` | Cosine hemisphere |
| `brdf_emissive_textured.glsl` | `vec3(0)` | `texture * intensity` | `vec3(0)` |

Each BRDF snippet implements three functions forward-declared by base templates:
- `evaluate_brdf(normal, material_address, wi, wo)` — spectral response for light direction pair
- `emissive_light(material_address)` — self-emitted light color
- `generate_ray(sample_index, xi, material_address, normal, tangent, bitangent)` — sampled ray direction (gated by `#ifdef BRDF_HAS_GENERATE_RAY`, only in G-Buffer context)

BRDFs are combined with `gbuffer_base.glsl` (rasterization) and `indirect_light_base.glsl` (ray tracing) to generate per-material shaders at runtime.
