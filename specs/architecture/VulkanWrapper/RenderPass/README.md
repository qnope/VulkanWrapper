# RenderPass Module

`vw` namespace · `RenderPass/` directory · Tier 6

Composable render pass system using dynamic rendering (no `VkRenderPass`/`VkFramebuffer`). Passes declare input/output slots and are wired together automatically by `RenderPipeline`.

## Slot (unified enum)

Global enum defining all image attachment points:

| Category | Slots |
|----------|-------|
| Geometry | `Depth` |
| G-Buffer | `Albedo`, `Normal`, `Tangent`, `Bitangent`, `Position`, `DirectLight`, `IndirectRay` |
| Post-process | `AmbientOcclusion`, `Sky`, `IndirectLight` |
| Final | `ToneMapped` |
| Extension | `UserSlot = 1024` |

## RenderPass (base class)

Non-templated base class with lazy image allocation:

- `input_slots()` / `output_slots()` — pure virtual, declare dependencies
- `execute(cmd, tracker, width, height, frame_index)` — pure virtual
- `get_or_create_image(slot, ...)` → `CachedImage` — cached by `(slot, width, height, frame_index)`, auto-evicts on resize
- `get_input(slot)` / `set_input(slot, image)` — wiring mechanism used by `RenderPipeline`
- `reset_accumulation()` — virtual, no-op by default (overridden by progressive passes)

## ScreenSpacePass (extends RenderPass)

Fullscreen quad rendering base for post-processing:

- `render_fullscreen(cmd, extent, colorAttachment, depthAttachment, pipeline, descriptorSet, pushConstants)` — handles `beginRendering`, viewport, scissor, bind, `draw(4, 1, 0, 0)`, `endRendering`
- `create_default_sampler()` — linear filtering, clamp-to-edge
- Helper: `create_screen_space_pipeline(device, vert, frag, layout, format, ...)` factory

## RenderPipeline (orchestrator)

Container executing passes in insertion order with automatic slot wiring:

```cpp
vw::RenderPipeline pipeline;
auto& zpass = pipeline.add(std::make_unique<vw::ZPass>(device, allocator, shader_dir));
auto& directLight = pipeline.add(std::make_unique<vw::DirectLightPass>(...));
pipeline.validate();  // checks all input slots are satisfied by predecessors
pipeline.execute(cmd, tracker, width, height, frame_index);
```

- `add<T>(unique_ptr<T>)` — returns typed reference
- `validate()` — returns errors if any pass has unfulfilled input slots
- `execute()` — wires predecessor outputs as inputs, executes all passes in order
- `reset_accumulation()` — propagates to all passes

## Concrete Passes

| Pass | Inputs | Outputs | Notes |
|------|--------|---------|-------|
| `ZPass` | — | `Depth` | Depth prepass, renders scene geometry |
| `DirectLightPass` | `Depth` | `Albedo`, `Normal`, `Tangent`, `Bitangent`, `Position`, `DirectLight`, `IndirectRay` | G-Buffer fill + per-fragment sun lighting via ray queries. Per-material fragment shaders from `gbuffer_base.glsl` + handler `brdf_path()` |
| `AmbientOcclusionPass` | `Depth`, `Position`, `Normal`, `Tangent`, `Bitangent` | `AmbientOcclusion` | SSAO via ray queries, progressive (1 sample/pixel/frame) |
| `SkyPass` | `Depth` | `Sky` | Atmospheric sky where depth == 1.0 |
| `IndirectLightPass` | `Position`, `Normal`, `Albedo`, `AmbientOcclusion`, `IndirectRay` | `IndirectLight` | RT indirect sky lighting, progressive accumulation. Per-material closest hit shaders from `indirect_light_base.glsl` + handler `brdf_path()` |
| `ToneMappingPass` | `Sky`, `DirectLight`, optionally `IndirectLight` | `ToneMapped` | HDR→LDR. Operators: `ACES`, `Reinhard`, `ReinhardExtended`, `Uncharted2`, `Neutral`. Also `execute_to_view()` for rendering to external image (swapchain) |

## SkyParameters / SkyParametersGPU

Sun and atmosphere configuration:
- `SkyParameters::create_earth_sun(angle)` — factory for Earth-like atmosphere
- `SkyParametersGPU` — GPU-layout struct for push constants (96 bytes)

## Pipeline Flow

```
ZPass → DirectLightPass → AO Pass → SkyPass → IndirectLightPass → ToneMappingPass → Swapchain
```
