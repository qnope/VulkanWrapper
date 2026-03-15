---
sidebar_position: 1
---

# 1. Hello Triangle

In this tutorial you will render a colored triangle to the screen using VulkanWrapper. Along the way you will learn the fundamental steps of every Vulkan application: creating geometry, uploading it to the GPU, compiling shaders, building a graphics pipeline, and recording draw commands.

![Blue triangle on a black background](/img/tutorials/triangle.png)

## Overview

The Triangle example lives in `examples/Triangle/main.cpp`. It inherits from **ExampleRunner**, a small base class that handles the window, swapchain, and frame loop so you can focus on rendering logic. You only need to override two methods:

- `setup()` -- called once, before the first frame.
- `render()` -- called every frame with a command buffer ready to record into.

Here is the minimal `main()`:

```cpp
int main() {
    App app;
    TriangleExample example(app);
    example.run();
}
```

`App` is a plain struct that creates the Vulkan instance, device, allocator, window, and swapchain in its member initializers. You do not need to write any boilerplate for those.

## Step 1 -- Define push constants

Push constants are a fast way to send small amounts of data (a few hundred bytes) to shaders without creating descriptor sets. Our triangle needs a Model-View-Projection matrix and a color:

```cpp
struct TrianglePushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};
```

## Step 2 -- Create geometry

Inside `setup()`, the example generates triangle vertices and indices with a built-in primitive helper:

```cpp
auto [vertices, indices] =
    vw::Model::create_triangle(
        vw::Model::PlanePrimitive::XY);
m_index_count =
    static_cast<uint32_t>(indices.size());
```

`create_triangle` returns a pair of `std::vector<FullVertex3D>` and `std::vector<uint32_t>`. `FullVertex3D` contains position, normal, tangent, bitangent, and UV -- everything a physically-based renderer might need. For this simple example we only use the position.

## Step 3 -- Allocate GPU buffers

VulkanWrapper provides type-safe buffer creation. The template parameters encode the element type, host-visibility, and usage flags at compile time:

```cpp
m_vertex_buffer.emplace(
    vw::allocate_vertex_buffer<vw::FullVertex3D, false>(
        *app().allocator, vertices.size()));
m_index_buffer.emplace(
    vw::create_buffer<uint32_t, false,
                      vw::IndexBufferUsage>(
        *app().allocator, indices.size()));
```

The `false` template argument means the buffers are **device-local** (fastest for the GPU but not directly writable from the CPU). We will use a staging buffer to upload the data.

## Step 4 -- Upload data with StagingBufferManager

Because the buffers are device-local, we copy data through a temporary host-visible staging buffer:

```cpp
{
    vw::StagingBufferManager staging(app().device,
                                     app().allocator);
    staging.fill_buffer(
        std::span<const vw::FullVertex3D>{vertices},
        *m_vertex_buffer, 0);
    staging.fill_buffer(
        std::span<const uint32_t>{indices},
        *m_index_buffer, 0);
    auto cmd = staging.fill_command_buffer();
    app().device->graphicsQueue().enqueue_command_buffer(
        cmd);
    app().device->graphicsQueue().submit({}, {}, {}).wait();
}
```

`StagingBufferManager` batches all your uploads into a single command buffer. After submitting, we call `wait()` to block until the GPU finishes copying. The staging buffer is destroyed when the scope exits.

## Step 5 -- Compile shaders

VulkanWrapper includes a runtime GLSL compiler powered by shaderc. No offline compilation step is needed:

```cpp
vw::ShaderCompiler compiler;
auto vert = compiler.compile_file_to_module(
    app().device,
    "../../../examples/Triangle/triangle.vert");
auto frag = compiler.compile_file_to_module(
    app().device,
    "../../../examples/Triangle/triangle.frag");
```

The vertex shader transforms each vertex by the MVP matrix:

```glsl
#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inUV;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
}
```

The fragment shader outputs a solid color from the push constants:

```glsl
#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec3 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pc.color, 1.0);
}
```

## Step 6 -- Build the pipeline layout and graphics pipeline

The **pipeline layout** describes what external data the pipeline can access. Here we declare a single push constant range:

```cpp
auto layout =
    vw::PipelineLayoutBuilder(app().device)
        .with_push_constant_range(
            {vk::ShaderStageFlagBits::eAllGraphics, 0,
             sizeof(TrianglePushConstants)})
        .build();
```

The **graphics pipeline** ties together shaders, vertex format, color attachments, and dynamic state:

```cpp
m_pipeline =
    vw::GraphicsPipelineBuilder(app().device,
                                std::move(layout))
        .add_shader(vk::ShaderStageFlagBits::eVertex,
                    vert)
        .add_shader(vk::ShaderStageFlagBits::eFragment,
                    frag)
        .add_vertex_binding<vw::FullVertex3D>()
        .add_color_attachment(app().swapchain.format())
        .with_dynamic_viewport_scissor()
        .build();
```

Key points:

- `add_vertex_binding<vw::FullVertex3D>()` automatically generates the correct `VkVertexInputAttributeDescription` entries for position, normal, tangent, bitangent, and UV.
- `add_color_attachment` tells the pipeline the format of the render target.
- `with_dynamic_viewport_scissor` means we set the viewport and scissor rectangle each frame instead of baking them into the pipeline. This is needed for proper window resizing.

## Step 7 -- Set up the camera

The last thing `setup()` does is compute the MVP matrix and store the color:

```cpp
auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f),
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));
float aspect =
    static_cast<float>(app().swapchain.width()) /
    static_cast<float>(app().swapchain.height());
auto proj = glm::perspective(glm::radians(60.0f), aspect,
                             0.1f, 10.0f);
proj[1][1] *= -1; // Vulkan Y flip

m_push_constants.mvp = proj * view;
m_push_constants.color = glm::vec3(0.2f, 0.6f, 1.0f);
```

The `proj[1][1] *= -1` line is important. Vulkan's clip-space Y axis points downward (opposite to OpenGL), so we flip the projection matrix to get the expected orientation.

## Step 8 -- Record draw commands

The `render()` method is called every frame. It receives a command buffer that is already begun and ready to record into.

### Transition the swapchain image

Before we can render to the swapchain image, we must transition it to the correct layout using `ResourceTracker`:

```cpp
transfer.resourceTracker().request(
    vw::Barrier::ImageState{
        .image = image_view->image()->handle(),
        .subresourceRange =
            image_view->subresource_range(),
        .layout =
            vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::
            eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::
            eColorAttachmentWrite});
transfer.resourceTracker().flush(cmd);
```

`ResourceTracker` automatically inserts the minimal pipeline barrier needed to transition the image from its current state to the requested state. You never need to manually specify `srcStageMask` or `srcAccessMask`.

### Begin dynamic rendering

VulkanWrapper uses **dynamic rendering** (Vulkan 1.3) instead of render pass objects. This means you describe attachments inline:

```cpp
vk::RenderingAttachmentInfo color_attachment{};
color_attachment.imageView = image_view->handle();
color_attachment.imageLayout =
    vk::ImageLayout::eColorAttachmentOptimal;
color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
color_attachment.clearValue.color = vk::ClearColorValue{
    std::array{0.0f, 0.0f, 0.0f, 1.0f}};

vk::RenderingInfo rendering_info{};
rendering_info.renderArea = vk::Rect2D{
    {0, 0}, {swapchain_width, swapchain_height}};
rendering_info.layerCount = 1;
rendering_info.colorAttachmentCount = 1;
rendering_info.pColorAttachments = &color_attachment;

cmd.beginRendering(rendering_info);
```

### Draw

With rendering active, we set the viewport and scissor, bind the pipeline, bind buffers, push constants, and issue the draw call:

```cpp
cmd.setViewport(0, viewport);
cmd.setScissor(0, scissor);

cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                 m_pipeline->handle());

vk::Buffer vb = m_vertex_buffer->handle();
vk::DeviceSize offset = 0;
cmd.bindVertexBuffers(0, vb, offset);
cmd.bindIndexBuffer(m_index_buffer->handle(), 0,
                    vk::IndexType::eUint32);

cmd.pushConstants<TrianglePushConstants>(
    m_pipeline->layout().handle(),
    vk::ShaderStageFlagBits::eAllGraphics, 0,
    m_push_constants);

cmd.drawIndexed(m_index_count, 1, 0, 0, 0);
cmd.endRendering();
```

`ExampleRunner` handles presenting the swapchain image and synchronizing frames for you.

## The complete class

Putting it all together, the full `TriangleExample` class with member variables and type aliases looks like this:

```cpp
class TriangleExample : public ExampleRunner {
    using VertexBuffer =
        vw::Buffer<vw::FullVertex3D, false, vw::VertexBufferUsage>;
    using IndexBuffer =
        vw::Buffer<uint32_t, false, vw::IndexBufferUsage>;

    std::optional<VertexBuffer> m_vertex_buffer;
    std::optional<IndexBuffer> m_index_buffer;
    std::shared_ptr<const vw::Pipeline> m_pipeline;
    TrianglePushConstants m_push_constants{};
    uint32_t m_index_count = 0;

  public:
    using ExampleRunner::ExampleRunner;
    // ... setup() and render() as described above
};
```

The buffers are wrapped in `std::optional` because they cannot be default-constructed -- they require an allocator. We fill them inside `setup()` with `emplace()`.

## What you learned

- **ExampleRunner** provides the boilerplate (window, swapchain, frame loop).
- **Type-safe buffers** (`allocate_vertex_buffer`, `create_buffer`) encode usage at compile time.
- **StagingBufferManager** batches CPU-to-GPU uploads into a single command buffer.
- **ShaderCompiler** compiles GLSL at runtime -- no offline step needed.
- **GraphicsPipelineBuilder** chains together all pipeline state with a fluent API.
- **ResourceTracker** handles image layout transitions automatically.
- **Dynamic rendering** replaces the traditional `VkRenderPass` / `VkFramebuffer` objects.

## Next steps

This example renders directly to the swapchain with a hand-written pipeline. In the [next tutorial](cube-shadow.md), you will use the **RenderPipeline** system and **ray-traced shadows** to render a 3D scene with multiple passes.
