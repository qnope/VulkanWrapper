---
sidebar_position: 1
---

# Graphics Pipeline

The `Pipeline` and `GraphicsPipelineBuilder` classes provide a fluent API for creating Vulkan graphics pipelines.

## Overview

```cpp
#include <VulkanWrapper/Pipeline/Pipeline.h>

auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragmentShader)
    .add_vertex_binding<FullVertex3D>()
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .with_dynamic_viewport_scissor()
    .build();
```

## GraphicsPipelineBuilder

### Constructor

```cpp
GraphicsPipelineBuilder(std::shared_ptr<const Device> device,
                        PipelineLayout pipelineLayout);
```

### Shader Methods

| Method | Description |
|--------|-------------|
| `add_shader(stage, module)` | Add a shader stage |

```cpp
builder.add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
       .add_shader(vk::ShaderStageFlagBits::eFragment, fragmentShader);
```

### Vertex Input

```cpp
// Add vertex binding (template uses Vertex concept)
builder.add_vertex_binding<FullVertex3D>();
builder.add_vertex_binding<ColoredVertex<glm::vec3>>();
```

### Attachments

| Method | Description |
|--------|-------------|
| `add_color_attachment(format)` | Add color attachment without blending |
| `add_color_attachment(format, blend)` | Add with blending config |
| `set_depth_format(format)` | Set depth attachment format |
| `set_stencil_format(format)` | Set stencil format |

```cpp
// No blending
builder.add_color_attachment(vk::Format::eR8G8B8A8Srgb);

// With alpha blending
builder.add_color_attachment(vk::Format::eR8G8B8A8Srgb,
                             ColorBlendConfig::alpha());

// Depth buffer
builder.set_depth_format(vk::Format::eD32Sfloat);
```

### Rasterization State

| Method | Description |
|--------|-------------|
| `with_topology(topology)` | Set primitive topology |
| `with_cull_mode(mode)` | Set culling mode |
| `with_depth_test(write, compare)` | Enable depth testing |

```cpp
builder.with_topology(vk::PrimitiveTopology::eTriangleList)
       .with_cull_mode(vk::CullModeFlagBits::eBack)
       .with_depth_test(true, vk::CompareOp::eLess);
```

### Viewport/Scissor

| Method | Description |
|--------|-------------|
| `with_fixed_viewport(w, h)` | Use fixed viewport |
| `with_fixed_scissor(w, h)` | Use fixed scissor |
| `with_dynamic_viewport_scissor()` | Use dynamic state |

```cpp
// Most common: dynamic viewport/scissor
builder.with_dynamic_viewport_scissor();

// Fixed viewport
builder.with_fixed_viewport(1920, 1080)
       .with_fixed_scissor(1920, 1080);
```

### Dynamic State

```cpp
builder.add_dynamic_state(vk::DynamicState::eViewport)
       .add_dynamic_state(vk::DynamicState::eScissor)
       .add_dynamic_state(vk::DynamicState::eBlendConstants);
```

## ColorBlendConfig

Predefined blending configurations:

```cpp
struct ColorBlendConfig {
    static ColorBlendConfig alpha();          // Standard alpha blend
    static ColorBlendConfig additive();       // Additive blend
    static ColorBlendConfig constant_blend(); // Dynamic constant blend
};
```

### Alpha Blending

```cpp
// result = src * srcAlpha + dst * (1 - srcAlpha)
builder.add_color_attachment(format, ColorBlendConfig::alpha());
```

### Additive Blending

```cpp
// result = src + dst
builder.add_color_attachment(format, ColorBlendConfig::additive());
```

### Constant Blending

```cpp
// result = src * C + dst * (1 - C)
// Set C with cmd.setBlendConstants()
builder.add_color_attachment(format, ColorBlendConfig::constant_blend());
```

## Pipeline Class

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `handle()` | `vk::Pipeline` | Get raw handle |
| `layout()` | `const PipelineLayout&` | Get pipeline layout |

### Usage

```cpp
// Bind pipeline
cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->handle());

// Bind descriptor sets
cmd.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline->layout().handle(),
    0,
    descriptorSet,
    {}
);
```

## Complete Example

```cpp
// Compile shaders
auto vertShader = ShaderCompiler::compile(
    device, "mesh.vert", vk::ShaderStageFlagBits::eVertex);
auto fragShader = ShaderCompiler::compile(
    device, "mesh.frag", vk::ShaderStageFlagBits::eFragment);

// Create layout
auto layout = PipelineLayoutBuilder()
    .setDevice(device)
    .addDescriptorSetLayout(descriptorLayout)
    .addPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex)
    .build();

// Create pipeline
auto pipeline = GraphicsPipelineBuilder(device, layout)
    // Shaders
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)

    // Vertex input
    .add_vertex_binding<FullVertex3D>()

    // G-buffer outputs
    .add_color_attachment(vk::Format::eR16G16B16A16Sfloat)  // Position
    .add_color_attachment(vk::Format::eR16G16B16A16Sfloat)  // Normal
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)        // Albedo

    // Depth
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)

    // State
    .with_cull_mode(vk::CullModeFlagBits::eBack)
    .with_dynamic_viewport_scissor()

    .build();
```

## Best Practices

1. **Use dynamic viewport/scissor** for flexibility
2. **Cache pipelines** - creation is expensive
3. **Use pipeline cache** for faster loading
4. **Match vertex input** to shader expectations
5. **Set appropriate blend modes** for each attachment
