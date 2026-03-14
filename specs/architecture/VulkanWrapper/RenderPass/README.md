# RenderPass Module

`vw` namespace · `RenderPass/` directory · Tier 6

Composable render pass system using dynamic rendering (no `VkRenderPass`/`VkFramebuffer`). Passes form a functional pipeline where each takes input image views and returns output image views.

## Subpass\<SlotEnum\> (base template)

Base class for all render passes with lazy image allocation:

- `SlotEnum` — enum defining output image slots
- `get_or_create_image(slot, width, height, frame_index, format, usage)` → `CachedImage`
- Images created on first use, cached by `(slot, width, height, frame_index)`
- Auto-evicts images when dimensions change (resize-safe)
- `CachedImage` = `{shared_ptr<Image>, shared_ptr<ImageView>}`

## ScreenSpacePass\<SlotEnum\> (extends Subpass)

Fullscreen quad rendering base for post-processing:

- `render_fullscreen(cmd, extent, colorAttachment, depthAttachment, pipeline, descriptorSet, pushConstants)` — handles all boilerplate: `beginRendering`, viewport, scissor, bind, `draw(4, 1, 0, 0)` (triangle strip), `endRendering`
- `create_default_sampler()` — linear filtering, clamp-to-edge
- Helper: `create_screen_space_pipeline(device, vert, frag, layout, format, ...)` factory

## SkyPass (extends ScreenSpacePass)

Atmospheric sky rendering where depth == 1.0 (far plane):
- Input: depth buffer view, `SkyParameters`, inverse view-projection matrix
- Output: HDR sky radiance image
- Push constants: `SkyParametersGPU` + `inverseViewProj`

## ToneMappingPass (extends ScreenSpacePass)

HDR to LDR conversion with multiple operators:
- Input: sky view, direct light view, optional indirect light view
- Output: LDR tone-mapped image (or renders to external view like swapchain)
- Operators: `ACES`, `Reinhard`, `ReinhardExtended`, `Uncharted2`, `Neutral`
- Push constants: exposure, operator_id, white_point, luminance_scale, indirect_intensity
- Gamma handled by sRGB output format, not the shader

## IndirectLightPass (extends Subpass, uses ray tracing)

Progressive indirect sky lighting via ray tracing pipeline:
- Input: position, normal, albedo, AO, indirect ray direction GBuffer views
- Output: accumulated indirect light storage image
- Per-material closest hit shaders generated dynamically from `indirect_light_base.glsl` + handler `brdf_path()`
- Progressive accumulation: 1 sample/pixel/frame, blended with history via `imageLoad`/`imageStore`
- `reset_accumulation()` — call on camera move or parameter change
- `get_frame_count()` — current accumulation sample count

## SkyParameters / SkyParametersGPU

Sun and atmosphere configuration:
- `SkyParameters::create_earth_sun(angle)` — factory for Earth-like atmosphere
- `SkyParametersGPU` — GPU-layout struct for push constants (96 bytes)

## Pipeline Flow (Advanced example)

```
ZPass → DirectLightPass → AO Pass → SkyPass → IndirectLightPass → ToneMappingPass → Swapchain
```
