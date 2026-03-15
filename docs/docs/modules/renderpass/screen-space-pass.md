---
sidebar_position: 2
title: "Screen-Space Pass"
---

# Screen-Space Pass

`ScreenSpacePass` is a convenience base class for render passes that draw a fullscreen quad. It extends `RenderPass` with helpers that eliminate the boilerplate of setting up viewports, scissors, dynamic rendering, descriptor binding, and push constants for screen-space post-processing effects.

Many of the built-in passes -- `SkyPass`, `ToneMappingPass`, and `AmbientOcclusionPass` -- inherit from `ScreenSpacePass`.

## Class Overview

```cpp
class ScreenSpacePass : public RenderPass {
public:
    using RenderPass::RenderPass; // inherit constructor

protected:
    std::shared_ptr<const Sampler> create_default_sampler();

    void render_fullscreen(
        vk::CommandBuffer cmd, vk::Extent2D extent,
        const vk::RenderingAttachmentInfo &color_attachment,
        const vk::RenderingAttachmentInfo *depth_attachment,
        const Pipeline &pipeline,
        std::optional<DescriptorSet> descriptor_set = std::nullopt,
        const void *push_constants = nullptr,
        size_t push_constants_size = 0);
};
```

`ScreenSpacePass` inherits the same constructor as `RenderPass`, so derived classes pass `device` and `allocator` through.

## create_default_sampler()

Creates a sampler suitable for most screen-space effects:

```cpp
std::shared_ptr<const Sampler> create_default_sampler();
```

This is a thin wrapper around `SamplerBuilder`:

```cpp
// Equivalent to:
SamplerBuilder(m_device).build();
```

The default sampler uses linear filtering and clamp-to-edge addressing, which works well for sampling G-Buffer textures and post-processing inputs.

Typical usage in a derived pass constructor:

```cpp
class MyPostProcess : public vw::ScreenSpacePass {
public:
    MyPostProcess(std::shared_ptr<vw::Device> device,
                  std::shared_ptr<vw::Allocator> allocator)
        : ScreenSpacePass(std::move(device), std::move(allocator))
        , m_sampler(create_default_sampler()) {}

private:
    std::shared_ptr<const vw::Sampler> m_sampler;
};
```

## render_fullscreen()

This is the workhorse method. It handles all the common rendering boilerplate for a fullscreen quad:

```cpp
void render_fullscreen(
    vk::CommandBuffer cmd,
    vk::Extent2D extent,
    const vk::RenderingAttachmentInfo &color_attachment,
    const vk::RenderingAttachmentInfo *depth_attachment,
    const Pipeline &pipeline,
    std::optional<DescriptorSet> descriptor_set = std::nullopt,
    const void *push_constants = nullptr,
    size_t push_constants_size = 0);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `cmd` | The command buffer to record into |
| `extent` | Width and height of the render area |
| `color_attachment` | Color attachment info (the image being written to) |
| `depth_attachment` | Optional depth attachment (pass `nullptr` to skip) |
| `pipeline` | The graphics pipeline to bind |
| `descriptor_set` | Optional descriptor set (textures, samplers, UBOs) |
| `push_constants` | Optional push constant data pointer |
| `push_constants_size` | Size of push constant data in bytes |

### What It Does

When called, `render_fullscreen()` records the following commands:

1. **Begin dynamic rendering** with the provided color (and optional depth) attachment.
2. **Set viewport and scissor** to cover the full extent.
3. **Bind the pipeline** as a graphics pipeline.
4. **Bind the descriptor set** (if provided) at set 0.
5. **Push constants** (if provided) to the fragment shader stage.
6. **Draw 4 vertices** as a triangle strip -- the fullscreen quad.
7. **End rendering**.

The fullscreen quad is generated entirely in the vertex shader using `gl_VertexIndex` (no vertex buffer needed). The companion shader `fullscreen.vert` computes clip-space positions and UV coordinates from vertex indices 0-3.

### Usage in execute()

Here is how a derived pass typically uses `render_fullscreen()` inside its `execute()` method:

```cpp
void MyPostProcess::execute(
    vk::CommandBuffer cmd,
    vw::Barrier::ResourceTracker &tracker,
    vw::Width width, vw::Height height,
    size_t frame_index) {

    // 1. Get or create the output image
    auto &output = get_or_create_image(
        vw::Slot::ToneMapped, width, height, frame_index,
        vk::Format::eB8G8R8A8Srgb,
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled);

    // 2. Transition images with ResourceTracker
    tracker.request(vw::Barrier::ImageState{
        .image = output.image->handle(),
        .range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite});
    tracker.flush(cmd);

    // 3. Set up the color attachment
    vk::RenderingAttachmentInfo color_attachment;
    color_attachment.setImageView(output.view->handle())
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    // 4. Render the fullscreen quad
    vk::Extent2D extent{width.value, height.value};
    render_fullscreen(cmd, extent, color_attachment, nullptr,
                      *m_pipeline, m_descriptor_set,
                      &m_push_constants,
                      sizeof(m_push_constants));
}
```

## create_screen_space_pipeline()

A free function that creates a graphics pipeline pre-configured for fullscreen quad rendering:

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

### Parameters

| Parameter | Description |
|-----------|-------------|
| `device` | Vulkan device |
| `vertex_shader` | Compiled vertex shader (typically `fullscreen.vert`) |
| `fragment_shader` | Compiled fragment shader for the effect |
| `descriptor_set_layout` | Layout describing the textures/buffers the shader reads |
| `color_format` | Output color attachment format |
| `depth_format` | Depth format (pass `eUndefined` to disable depth) |
| `depth_compare_op` | Depth comparison operator (default: `eAlways`) |
| `push_constants` | Push constant ranges for the pipeline layout |
| `blend` | Optional blending configuration |

### What It Configures

The returned pipeline is set up with:

- **Triangle strip topology** for the fullscreen quad (4 vertices, no vertex buffer)
- **Dynamic viewport and scissor** (set at draw time)
- **Single color attachment** with optional blending
- **Optional depth testing** (disabled when `depth_format` is `eUndefined`)

### ColorBlendConfig Presets

For the `blend` parameter, `ColorBlendConfig` provides static factory methods:

```cpp
// Progressive accumulation: result = src * C + dst * (1-C)
// Set C at draw time with cmd.setBlendConstants()
ColorBlendConfig::constant_blend();

// Standard alpha blending: result = src * srcAlpha + dst * (1-srcAlpha)
ColorBlendConfig::alpha();

// Additive blending: result = src + dst
ColorBlendConfig::additive();
```

### Example: Creating a Post-Processing Pipeline

```cpp
vw::ShaderCompiler compiler;

auto vert = compiler.compile_file_to_module(
    device, shader_dir / "fullscreen.vert");
auto frag = compiler.compile_file_to_module(
    device, shader_dir / "my_effect.frag");

// Define push constant range for fragment shader
vk::PushConstantRange push_range{
    vk::ShaderStageFlagBits::eFragment,
    0,
    sizeof(MyPushConstants)};

auto pipeline = vw::create_screen_space_pipeline(
    device, vert, frag,
    descriptor_set_layout,
    vk::Format::eR16G16B16A16Sfloat,   // HDR output
    vk::Format::eUndefined,             // no depth
    vk::CompareOp::eAlways,
    {push_range});
```

## Fullscreen Vertex Shader

The `fullscreen.vert` shader that ships with VulkanWrapper generates a fullscreen quad from vertex indices alone. It outputs positions and UVs for 4 vertices drawn as a triangle strip:

| Vertex Index | Position | UV |
|:---:|:---:|:---:|
| 0 | (-1, -1) | (0, 0) |
| 1 | (+1, -1) | (1, 0) |
| 2 | (-1, +1) | (0, 1) |
| 3 | (+1, +1) | (1, 1) |

No vertex buffer is needed. The draw call is simply `cmd.draw(4, 1, 0, 0)`.

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RenderPass/ScreenSpacePass.h` | `ScreenSpacePass` class, `create_screen_space_pipeline()` |
| `VulkanWrapper/Pipeline/Pipeline.h` | `ColorBlendConfig` struct |
