# Pipeline Module

`vw` namespace · `Pipeline/` directory · Tier 4

Graphics and compute pipeline construction via builders. All pipelines use dynamic rendering (no `VkRenderPass`).

## Pipeline

Owns a `vk::UniquePipeline` + `PipelineLayout`. Derives from `ObjectWithUniqueHandle`.

## GraphicsPipelineBuilder

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_shader(eVertex, vertModule)
    .add_shader(eFragment, fragModule)
    .add_vertex_binding<Vertex3D>()
    .add_color_attachment(format, ColorBlendConfig::additive())
    .set_depth_format(eD32Sfloat)
    .with_depth_test(true, eLess)
    .with_dynamic_viewport_scissor()
    .with_topology(eTriangleList)
    .with_cull_mode(eBack)
    .build();
```

## ColorBlendConfig

Blending presets via static factory methods:
- `constant_blend()` — `result = src * C + dst * (1-C)`, dynamic constants via `cmd.setBlendConstants()`
- `alpha()` — standard alpha blending
- `additive()` — `result = src + dst`

## PipelineLayout

Wraps descriptor set layouts + push constant ranges. Created implicitly by pipeline builders.

## ShaderModule

Wraps `vk::UniqueShaderModule`. Created by `ShaderCompiler::compile_file_to_module()`.

## ComputePipeline

Compute-only pipeline for dispatch operations.

## MeshRenderer

Note: lives in the **Model** module (not Pipeline) to avoid circular dependencies. Binds pipelines per `MaterialTypeTag` for bindless mesh rendering.

## Helper

`create_screen_space_pipeline()` (in `ScreenSpacePass.h`) creates fullscreen quad pipelines with triangle strip topology, dynamic viewport/scissor, and optional blending/depth.
