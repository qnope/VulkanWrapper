# Advanced Example

`examples/Advanced/` directory

Full deferred rendering pipeline with physically-based lighting, ambient occlusion, and ray-traced indirect illumination. Renders the Sponza scene and saves a screenshot after 32 AO samples.

## Pipeline Flow

Managed by `DeferredRenderingManager`:

```
1. ZPass              → Depth prepass
2. DirectLightPass    → G-Buffer fill + per-fragment sun lighting
3. AO Pass            → Screen-space ambient occlusion (progressive, 1 sample/frame)
4. SkyPass            → Atmospheric sky where depth == 1.0
5. IndirectLightPass  → RT indirect sky lighting (progressive accumulation)
6. ToneMappingPass    → HDR → LDR (ACES, exposure, white point)
7. Blit to swapchain
```

Each pass takes input image views and returns output views. DeferredRenderingManager chains them functionally, passing outputs from one pass as inputs to the next.

## Key Files

| File | Purpose |
|------|---------|
| `main.cpp` | Render loop, screenshot capture, swapchain recreation |
| `DeferredRenderingManager.h` | Orchestrates all passes |
| `ZPass.h` | Depth prepass (custom, not in library) |
| `DirectLightPass.h` | G-Buffer + sun lighting (custom) |
| `AmbientOcclusionPass.h` | SSAO via ray queries (custom, progressive) |
| `SceneSetup.h` | Sponza scene loading configuration |
| `RenderPassInformation.h` | UBO and camera structures |

## Custom Shaders (`Shaders/`)

- `GBuffer/gbuffer.vert`, `gbuffer_base.glsl`, `zpass.vert` — geometry pass shaders
- `post-process/ambient_occlusion.frag` — SSAO fragment shader
- `RayTracing/raygen.rgen`, `miss.rmiss`, `hit.rchit` — indirect light RT shaders
- `quad.vert` — screen-space quad vertex shader

## Running

`cd build/examples/Advanced && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./Main`

Outputs `screenshot.png` after 32 progressive AO samples.
