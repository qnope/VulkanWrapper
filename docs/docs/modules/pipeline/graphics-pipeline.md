---
sidebar_position: 1
title: "Graphics Pipeline"
---

# Graphics Pipeline

## Overview

A **graphics pipeline** encodes the entire GPU state for rasterization: shaders,
vertex format, viewport configuration, depth testing, blending, and more.
In Vulkan, pipelines are immutable once created -- you cannot change a setting
at draw time unless it was declared *dynamic*.

VulkanWrapper provides:

- **`GraphicsPipelineBuilder`** -- a fluent builder that accumulates state and
  produces a shared, immutable `Pipeline`.
- **`Pipeline`** -- an RAII wrapper that owns a `vk::UniquePipeline` and
  references its `PipelineLayout`.
- **`ColorBlendConfig`** -- a small struct with static factory methods for
  common blending presets.
- **`create_screen_space_pipeline()`** -- a convenience free function for the
  very common case of a fullscreen post-processing pass.

### Where it lives in the library

| Item | Header |
|------|--------|
| `GraphicsPipelineBuilder` | `VulkanWrapper/Pipeline/Pipeline.h` |
| `Pipeline` | `VulkanWrapper/Pipeline/Pipeline.h` |
| `ColorBlendConfig` | `VulkanWrapper/Pipeline/Pipeline.h` |
| `create_screen_space_pipeline` | `VulkanWrapper/RenderPass/ScreenSpacePass.h` |

All types live in the `vw` namespace.

---

## API Reference

### GraphicsPipelineBuilder

| Method | Returns | Description |
|--------|---------|-------------|
| `GraphicsPipelineBuilder(std::shared_ptr<const Device> device, PipelineLayout pipelineLayout)` | -- | Constructs the builder with a device and a pre-built pipeline layout. |
| `add_shader(vk::ShaderStageFlagBits flags, std::shared_ptr<const ShaderModule> module)` | `GraphicsPipelineBuilder&` | Attach a compiled shader to the pipeline. Typically called twice (vertex + fragment). |
| `add_dynamic_state(vk::DynamicState state)` | `GraphicsPipelineBuilder&` | Mark a piece of pipeline state as dynamic (set at draw time via command buffer). |
| `with_fixed_viewport(int width, int height)` | `GraphicsPipelineBuilder&` | Bake a fixed viewport into the pipeline. |
| `with_fixed_scissor(int width, int height)` | `GraphicsPipelineBuilder&` | Bake a fixed scissor rect into the pipeline. |
| `with_dynamic_viewport_scissor()` | `GraphicsPipelineBuilder&` | Declare both viewport and scissor as dynamic. You must call `cmd.setViewport()` and `cmd.setScissor()` before drawing. |
| `add_color_attachment(vk::Format format, std::optional<ColorBlendConfig> blend = std::nullopt)` | `GraphicsPipelineBuilder&` | Declare a color attachment. Call once per attachment. Pass a `ColorBlendConfig` to enable blending. |
| `set_depth_format(vk::Format format)` | `GraphicsPipelineBuilder&` | Set the depth attachment format (e.g., `vk::Format::eD32Sfloat`). |
| `set_stencil_format(vk::Format format)` | `GraphicsPipelineBuilder&` | Set the stencil attachment format. |
| `add_vertex_binding<V>()` | `GraphicsPipelineBuilder&` | Register vertex input bindings and attributes from a type that satisfies the `Vertex` concept. Binding and location indices are assigned automatically. |
| `with_depth_test(bool write, vk::CompareOp compare_operator)` | `GraphicsPipelineBuilder&` | Enable depth testing. `write` controls depth writes; `compare_operator` controls the comparison function. |
| `with_topology(vk::PrimitiveTopology topology)` | `GraphicsPipelineBuilder&` | Set primitive topology. Default is `eTriangleList`. |
| `with_cull_mode(vk::CullModeFlags cull_mode)` | `GraphicsPipelineBuilder&` | Set face culling. Default is `eBack`. |
| `build()` | `std::shared_ptr<const Pipeline>` | Create the immutable Vulkan pipeline object. |

**Default values** (when a method is not called):

| Setting | Default |
|---------|---------|
| Topology | `vk::PrimitiveTopology::eTriangleList` |
| Cull mode | `vk::CullModeFlagBits::eBack` |
| Depth test | Disabled |
| Depth compare op | `vk::CompareOp::eLess` |
| Depth format | `vk::Format::eUndefined` (no depth) |
| Stencil format | `vk::Format::eUndefined` (no stencil) |

### Pipeline

| Method | Returns | Description |
|--------|---------|-------------|
| `handle()` | `vk::Pipeline` | The raw Vulkan handle (inherited from `ObjectWithUniqueHandle`). |
| `layout()` | `const PipelineLayout&` | The pipeline layout used to create this pipeline. Needed when binding descriptor sets or pushing constants. |

### ColorBlendConfig

`ColorBlendConfig` is a plain struct defined in `Pipeline.h`. It has three
static factory methods for common presets:

| Static Factory | Formula | Notes |
|----------------|---------|-------|
| `ColorBlendConfig::constant_blend()` | `result = src * C + dst * (1 - C)` | C is set at draw time via `cmd.setBlendConstants()`. Good for fade/accumulation effects. Sets `useDynamicConstants = true`. |
| `ColorBlendConfig::alpha()` | `result = src * srcAlpha + dst * (1 - srcAlpha)` | Standard transparency blending. |
| `ColorBlendConfig::additive()` | `result = src + dst` | Used for light accumulation, particle effects, etc. |

You can also construct a `ColorBlendConfig` directly with custom blend factors
using C++23 designated initializers:

```cpp
vw::ColorBlendConfig custom{
    .srcColorBlendFactor = vk::BlendFactor::eOne,
    .dstColorBlendFactor = vk::BlendFactor::eOne,
    .colorBlendOp        = vk::BlendOp::eMax,
    .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
    .alphaBlendOp        = vk::BlendOp::eAdd,
    .useDynamicConstants = false};
```

### create_screen_space_pipeline()

```cpp
std::shared_ptr<const Pipeline> create_screen_space_pipeline(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const ShaderModule> vertex_shader,
    std::shared_ptr<const ShaderModule> fragment_shader,
    std::shared_ptr<const DescriptorSetLayout> descriptor_set_layout,
    vk::Format color_format,
    vk::Format depth_format = vk::Format::eUndefined,
    vk::CompareOp depth_compare_op = vk::CompareOp::eAlways,
    std::vector<vk::PushConstantRange> push_constants = {},
    std::optional<ColorBlendConfig> blend = std::nullopt);
```

This free function (declared in `ScreenSpacePass.h`) creates a pipeline
pre-configured for fullscreen screen-space rendering:

- **Triangle strip** topology (4 vertices for a fullscreen quad).
- **Dynamic viewport and scissor**.
- **No vertex input** (the vertex shader generates positions from
  `gl_VertexIndex`).
- Single color attachment with optional blending.
- Optional depth testing.

---

## Usage Examples

### Minimal triangle pipeline

```cpp
// 1. Compile shaders
vw::ShaderCompiler compiler;
auto vert = compiler.compile_file_to_module(device, "triangle.vert");
auto frag = compiler.compile_file_to_module(device, "triangle.frag");

// 2. Build pipeline layout (empty -- no descriptors, no push constants)
auto layout = vw::PipelineLayoutBuilder(device).build();

// 3. Build the pipeline
auto pipeline =
    vw::GraphicsPipelineBuilder(device, layout)
        .add_shader(vk::ShaderStageFlagBits::eVertex, vert)
        .add_shader(vk::ShaderStageFlagBits::eFragment, frag)
        .add_vertex_binding<vw::ColoredVertex3D>()
        .add_color_attachment(vk::Format::eR8G8B8A8Unorm)
        .with_dynamic_viewport_scissor()
        .build();
```

### Pipeline with depth testing and alpha blending

```cpp
auto pipeline =
    vw::GraphicsPipelineBuilder(device, layout)
        .add_shader(vk::ShaderStageFlagBits::eVertex, vert)
        .add_shader(vk::ShaderStageFlagBits::eFragment, frag)
        .add_vertex_binding<vw::FullVertex3D>()
        .add_color_attachment(vk::Format::eR8G8B8A8Unorm,
                              vw::ColorBlendConfig::alpha())
        .set_depth_format(vk::Format::eD32Sfloat)
        .with_depth_test(true, vk::CompareOp::eLess)
        .with_dynamic_viewport_scissor()
        .build();
```

### G-buffer pipeline with multiple color attachments

```cpp
auto pipeline =
    vw::GraphicsPipelineBuilder(device, layout)
        .add_shader(vk::ShaderStageFlagBits::eVertex, vert)
        .add_shader(vk::ShaderStageFlagBits::eFragment, frag)
        .add_vertex_binding<vw::FullVertex3D>()
        .add_color_attachment(vk::Format::eR16G16B16A16Sfloat)  // Position
        .add_color_attachment(vk::Format::eR16G16B16A16Sfloat)  // Normal
        .add_color_attachment(vk::Format::eR8G8B8A8Unorm)       // Albedo
        .set_depth_format(vk::Format::eD32Sfloat)
        .with_depth_test(true, vk::CompareOp::eLess)
        .with_cull_mode(vk::CullModeFlagBits::eBack)
        .with_dynamic_viewport_scissor()
        .build();
```

### Screen-space post-processing pipeline

```cpp
vw::ShaderCompiler compiler;
auto vert = compiler.compile_to_module(
    device, fullscreenVertSrc, vk::ShaderStageFlagBits::eVertex);
auto frag = compiler.compile_to_module(
    device, tonemappingFragSrc, vk::ShaderStageFlagBits::eFragment);

auto descriptorLayout =
    vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();

auto pipeline = vw::create_screen_space_pipeline(
    device, vert, frag, descriptorLayout,
    vk::Format::eR8G8B8A8Unorm);
```

### Screen-space pipeline with push constants and depth

```cpp
std::vector<vk::PushConstantRange> pushConstants = {
    vk::PushConstantRange(
        vk::ShaderStageFlagBits::eFragment, 0, sizeof(ToneMappingParams))};

auto pipeline = vw::create_screen_space_pipeline(
    device, vert, frag, descriptorLayout,
    vk::Format::eR8G8B8A8Unorm,
    vk::Format::eD32Sfloat,        // depth format
    vk::CompareOp::eLess,          // depth compare op
    pushConstants);                 // push constant ranges
```

### Drawing with the pipeline

```cpp
cmd.beginRendering(renderingInfo);

// Set dynamic viewport and scissor
vk::Viewport viewport(0.f, 0.f, float(width), float(height), 0.f, 1.f);
vk::Rect2D   scissor({0, 0}, {width, height});
cmd.setViewport(0, 1, &viewport);
cmd.setScissor(0, 1, &scissor);

// Bind the pipeline
cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->handle());

// Bind descriptor sets (using the layout from the pipeline)
auto descriptorHandle = descriptorSet.handle();
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline->layout().handle(),
    0, 1, &descriptorHandle, 0, nullptr);

// Draw
cmd.draw(vertexCount, 1, 0, 0);

cmd.endRendering();
```

---

## Design Rationale

### Why pipelines are `shared_ptr<const Pipeline>`

Pipelines are expensive to create (the driver may JIT-compile shaders).
Returning a `shared_ptr<const>` encourages sharing the same pipeline object
across multiple render passes and frames.  The `const` qualifier makes it
clear that the pipeline is immutable after creation.

### Why there is no `set_vertex_binding()` -- only `add_vertex_binding<V>()`

The builder uses the `Vertex` C++23 concept to automatically extract binding
and attribute descriptions from your vertex struct.  This eliminates a large
class of offset/format mismatch bugs.  See [Descriptor Sets](../descriptors/descriptor-sets.md)
for the `Vertex` concept definition and built-in vertex types.

### Dynamic rendering only

VulkanWrapper targets Vulkan 1.3 and uses `cmd.beginRendering()` exclusively.
There are no `VkRenderPass` or `VkFramebuffer` objects.  This simplifies
pipeline creation (no render pass compatibility requirements) and makes dynamic
resolution changes trivial.

---

## Common Pitfalls

1. **Mismatched color attachment count.**
   The number of `add_color_attachment()` calls must match the number of color
   attachments in your `vk::RenderingInfo`.  A mismatch causes validation
   errors or GPU hangs.

2. **Forgetting `with_dynamic_viewport_scissor()`.**
   If you neither set a fixed viewport/scissor nor declare them dynamic, the
   pipeline will have zero-size viewport and scissor, and nothing will be
   rendered.

3. **Using the wrong `PipelineLayout`.**
   The layout passed to the builder must match the descriptor set layouts and
   push constant ranges that you use at draw time.  If you change the layout
   later, you need to rebuild the pipeline.

4. **Not setting topology for non-triangle-list draws.**
   The default topology is `eTriangleList`.  If you draw a fullscreen quad with
   a triangle strip, you must call `with_topology(vk::PrimitiveTopology::eTriangleStrip)`.
   (Or just use `create_screen_space_pipeline()` which does this for you.)

5. **Blend configuration ignored without `add_color_attachment` blend parameter.**
   Passing `std::nullopt` (the default) disables blending for that attachment.
   If you need transparency, explicitly pass `ColorBlendConfig::alpha()`.

6. **Color format mismatch.**
   The format passed to `add_color_attachment()` must match the format of the
   image view used in `vk::RenderingAttachmentInfo`.  Mismatched formats lead
   to undefined behavior.
