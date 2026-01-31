---
sidebar_position: 1
---

# Hello Triangle

Learn the basics of VulkanWrapper by rendering a simple triangle.

## Prerequisites

- VulkanWrapper built and installed
- Basic C++ knowledge
- Understanding of graphics concepts

## Project Setup

Create a new file `main.cpp`:

```cpp
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Window/Window.h>
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>
#include <VulkanWrapper/Command/CommandPool.h>
#include <VulkanWrapper/Synchronization/Fence.h>
#include <VulkanWrapper/Synchronization/Semaphore.h>

using namespace vw;
```

## Step 1: Initialize SDL and Create Window

```cpp
int main() {
    // Initialize SDL
    SDL_Initializer sdl;

    // Create window
    auto window = WindowBuilder()
        .setWidth(1280)
        .setHeight(720)
        .setTitle("Hello Triangle")
        .build();
```

## Step 2: Create Vulkan Instance

```cpp
    // Create Vulkan instance with validation
    auto instance = InstanceBuilder()
        .setDebug()
        .setApiVersion(ApiVersion::e13)
        .addExtensions(window->vulkan_extensions())
        .build();
```

## Step 3: Create Device

```cpp
    // Create surface for presentation
    auto surface = window->create_surface(instance);

    // Find GPU and create device
    auto device = instance->findGpu()
        .requireGraphicsQueue()
        .requirePresentQueue(surface)
        .requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .find()
        .build();
```

## Step 4: Create Allocator and Swapchain

```cpp
    // Create memory allocator
    auto allocator = AllocatorBuilder()
        .setInstance(instance)
        .setDevice(device)
        .build();

    // Create swapchain
    auto swapchain = SwapchainBuilder()
        .setDevice(device)
        .setSurface(surface)
        .setExtent(Width{1280}, Height{720})
        .setPresentMode(vk::PresentModeKHR::eFifo)
        .build();
```

## Step 5: Create Shaders

Create `triangle.vert`:
```glsl
#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

Create `triangle.frag`:
```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

## Step 6: Create Pipeline

```cpp
    // Compile shaders
    auto vertShader = ShaderCompiler::compile(
        device, "triangle.vert", vk::ShaderStageFlagBits::eVertex);
    auto fragShader = ShaderCompiler::compile(
        device, "triangle.frag", vk::ShaderStageFlagBits::eFragment);

    // Create pipeline layout (no descriptors needed)
    auto pipelineLayout = PipelineLayoutBuilder()
        .setDevice(device)
        .build();

    // Create graphics pipeline
    auto pipeline = GraphicsPipelineBuilder(device, pipelineLayout)
        .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
        .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
        .add_color_attachment(swapchain->format())
        .with_dynamic_viewport_scissor()
        .build();
```

## Step 7: Create Command Pool and Buffers

```cpp
    // Create command pool
    auto commandPool = CommandPoolBuilder()
        .setDevice(device)
        .setQueueFamilyIndex(device->graphics_queue().family_index())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .build();

    // Allocate command buffers (one per swapchain image)
    auto commandBuffers = commandPool->allocate(swapchain->image_count());
```

## Step 8: Create Synchronization Objects

```cpp
    // Create sync objects
    auto imageAvailable = SemaphoreBuilder().setDevice(device).build();
    auto renderFinished = SemaphoreBuilder().setDevice(device).build();
    auto inFlightFence = FenceBuilder()
        .setDevice(device)
        .setSignaled(true)
        .build();
```

## Step 9: Render Loop

```cpp
    bool running = true;
    while (running) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Wait for previous frame
        inFlightFence->wait();
        inFlightFence->reset();

        // Acquire swapchain image
        uint32_t imageIndex = swapchain->acquire_next_image(*imageAvailable);

        // Record commands
        auto& cmd = commandBuffers[imageIndex];
        cmd.reset();

        {
            CommandBufferRecorder recorder(cmd);

            // Begin rendering
            vk::RenderingAttachmentInfo colorAttachment{
                .imageView = swapchain->image_views()[imageIndex].handle(),
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}}
            };

            vk::RenderingInfo renderInfo{
                .renderArea = {{0, 0}, swapchain->extent()},
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachment
            };

            cmd.handle().beginRendering(renderInfo);

            // Set viewport and scissor
            vk::Viewport viewport{
                0, 0,
                static_cast<float>(swapchain->extent().width),
                static_cast<float>(swapchain->extent().height),
                0, 1
            };
            cmd.handle().setViewport(0, viewport);

            vk::Rect2D scissor{{0, 0}, swapchain->extent()};
            cmd.handle().setScissor(0, scissor);

            // Bind pipeline and draw
            cmd.handle().bindPipeline(vk::PipelineBindPoint::eGraphics,
                                      pipeline->handle());
            cmd.handle().draw(3, 1, 0, 0);

            cmd.handle().endRendering();
        }

        // Submit
        vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = cmd.handle()};
        vk::SemaphoreSubmitInfo waitInfo{
            .semaphore = imageAvailable->handle(),
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
        };
        vk::SemaphoreSubmitInfo signalInfo{
            .semaphore = renderFinished->handle()
        };
        vk::SubmitInfo2 submitInfo{
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmdInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalInfo
        };

        device->graphics_queue().submit(submitInfo, inFlightFence->handle());

        // Present
        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinished->handle(),
            .swapchainCount = 1,
            .pSwapchains = &swapchain->handle(),
            .pImageIndices = &imageIndex
        };

        device->present_queue().present(presentInfo);
    }

    device->wait_idle();
    return 0;
}
```

## Result

You should see a window with a colorful triangle:
- Red vertex at top
- Green vertex at bottom right
- Blue vertex at bottom left
- Colors interpolated across the triangle

## Next Steps

- Add vertex buffers for custom geometry
- Implement camera controls
- Add texturing
- Explore the [Deferred Rendering](./deferred-rendering) tutorial
