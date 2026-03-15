---
sidebar_position: 1
---

# Introduction

**VulkanWrapper** is a modern C++23 library that provides high-level abstractions over the Vulkan 1.3 graphics API. It makes Vulkan development more accessible while preserving full access to advanced features like hardware ray tracing, dynamic rendering, and Synchronization2.

## Why VulkanWrapper?

Vulkan is the industry standard for high-performance, cross-platform GPU programming. It gives you fine-grained control over the graphics hardware -- but that control comes at a cost:

- **Verbose initialization**: creating a single triangle on screen requires hundreds of lines of boilerplate.
- **Manual resource lifetime management**: every buffer, image, and pipeline must be explicitly created and destroyed in the right order.
- **Complex synchronization**: you must insert memory barriers by hand to prevent data races between the CPU and GPU, and between different GPU operations.
- **Easy to misuse**: many errors are silent without validation layers, leading to crashes or rendering corruption.

VulkanWrapper addresses each of these challenges without hiding the underlying Vulkan model.

## Key Features

### RAII Resource Management

All Vulkan resources are automatically cleaned up when they go out of scope. You never call `vkDestroy*` or `vkFree*` manually:

```cpp
{
    auto instance = vw::InstanceBuilder()
        .setApiVersion(vw::ApiVersion::e13)
        .setDebug()
        .build();

    auto device = instance->findGpu()
        .with_queue(vk::QueueFlagBits::eGraphics)
        .with_synchronization_2()
        .with_dynamic_rendering()
        .build();

    auto allocator = vw::AllocatorBuilder(instance, device).build();

    // Use instance, device, allocator...
}  // Everything is automatically destroyed in the correct order
```

### Type-Safe Buffers

Buffer types encode their element type, host visibility, and usage flags at compile time. The compiler catches misuse before you ever run the program:

```cpp
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>

// Create a host-visible uniform buffer of 10 mat4 matrices
auto uniforms = vw::create_buffer<glm::mat4, true, vw::UniformBufferUsage>(
    *allocator, 10);

// Write data -- only compiles for host-visible buffers
uniforms.write(matrices, 0);

// Create a device-local vertex buffer
auto vertices = vw::allocate_vertex_buffer<Vertex, false>(*allocator, 1000);
// vertices.write(data, 0);  // Compile error! Device-local buffers are not writable from CPU
```

### Fluent Builder APIs

Complex Vulkan objects are constructed through intuitive, chainable builders:

```cpp
auto pipeline = vw::GraphicsPipelineBuilder(device, pipelineLayout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragmentShader)
    .add_vertex_binding<Vertex>()
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .with_dynamic_viewport_scissor()
    .build();
```

### Automatic Synchronization

The `ResourceTracker` in `vw::Barrier` tracks the state of every GPU resource and generates optimal pipeline barriers for you. No more guessing which stage flags or access masks to use:

```cpp
vw::Barrier::ResourceTracker tracker;

// Tell the tracker the current state of an image
tracker.track(vw::Barrier::ImageState{
    .image = myImage,
    .subresourceRange = fullRange,
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// Request a transition to a new state
tracker.request(vw::Barrier::ImageState{
    .image = myImage,
    .subresourceRange = fullRange,
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});

// Flush -- the correct vkCmdPipelineBarrier2 call is generated for you
tracker.flush(commandBuffer);
```

### Runtime Shader Compilation

Compile GLSL shaders to SPIR-V at runtime with full `#include` support:

```cpp
auto compiler = vw::ShaderCompiler()
    .add_include_path("Shaders/include")
    .add_macro("MAX_LIGHTS", "16");

auto vertModule = compiler.compile_file_to_module(device, "shader.vert");
auto fragModule = compiler.compile_file_to_module(device, "shader.frag");
```

### Hardware Ray Tracing

Build acceleration structures and ray tracing pipelines using the same builder pattern:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .with_ray_tracing()        // Enables VK_KHR_ray_tracing_pipeline
    .with_descriptor_indexing() // Required for bindless resources
    .build();
```

### Strong Types

Wrapper types like `Width`, `Height`, `MipLevel`, and `Depth` prevent parameter mix-ups at compile time:

```cpp
// The compiler won't let you pass a Height where a Width is expected
auto image = allocator->create_image_2D(
    vw::Width{1024}, vw::Height{768}, false,
    vk::Format::eR8G8B8A8Srgb,
    vk::ImageUsageFlagBits::eColorAttachment);
```

### Structured Error Handling

A rich exception hierarchy with `std::source_location` captures exactly where errors occur:

```cpp
// Each subsystem has its own exception type
try {
    auto result = check_vk(vulkanResult, "creating pipeline");
} catch (const vw::VulkanException &e) {
    // Knows the VkResult, context message, and source file/line
}
```

## What VulkanWrapper is NOT

- **Not a game engine.** It does not provide a scene graph, entity system, physics, or audio. It is a Vulkan abstraction layer.
- **Not a renderer.** The examples include rendering code, but the library itself is agnostic about your rendering architecture.
- **Not hiding Vulkan.** You still work with `vk::CommandBuffer`, `vk::Image`, and other Vulkan types. VulkanWrapper wraps the tedious parts while leaving you in control.

## Project Structure

```
VulkanWrapper/
├── VulkanWrapper/                     # Main library
│   ├── include/VulkanWrapper/         # Public headers
│   │   ├── Command/                   # CommandPool, CommandBuffer
│   │   ├── Descriptors/               # DescriptorSet, DescriptorSetLayout, Vertex
│   │   ├── Image/                     # Image, ImageView, Sampler, Mipmap
│   │   ├── Memory/                    # Allocator, Buffer, Transfer, StagingBufferManager
│   │   ├── Model/                     # Scene, Mesh, Materials (bindless)
│   │   ├── Pipeline/                  # GraphicsPipelineBuilder, PipelineLayout
│   │   ├── Random/                    # NoiseTexture, RandomSamplingBuffer
│   │   ├── RayTracing/                # BLAS, TLAS, RayTracingPipeline
│   │   ├── RenderPass/                # Subpass, ScreenSpacePass, SkyPass, ToneMappingPass
│   │   ├── Shader/                    # ShaderCompiler (GLSL to SPIR-V)
│   │   ├── Synchronization/           # ResourceTracker, Fence, Semaphore
│   │   ├── Utils/                     # Error (exception hierarchy), helpers
│   │   ├── Vulkan/                    # Instance, Device, DeviceFinder, Queue, Swapchain
│   │   └── Window/                    # SDL3 Window, SDL_Initializer
│   ├── src/                           # Implementation files (mirrors include/)
│   └── tests/                         # GTest unit tests
├── examples/
│   ├── Application/                   # Base application framework (App class)
│   ├── Common/                        # Shared example utilities (ExampleRunner)
│   ├── Triangle/                      # Minimal triangle rendering
│   ├── CubeShadow/                    # Cube with shadow mapping
│   ├── AmbientOcclusion/              # Screen-space ambient occlusion
│   ├── EmissiveCube/                  # Emissive material example
│   └── Advanced/                      # Deferred rendering + ray tracing
├── Shaders/                           # GLSL shaders and includes
└── Models/                            # 3D assets (Sponza scene)
```

## Next Steps

- **[Getting Started](./getting-started)** -- install prerequisites, build the project, and run your first example.
- **[Vulkan Primer](./vulkan-primer)** -- if you are new to Vulkan, start here to understand the concepts VulkanWrapper builds upon.
- **[Core Concepts](./category/core-concepts)** -- deep dives into the builder pattern, RAII ownership, type safety, and error handling.
- **[API Reference](./category/api-reference)** -- detailed documentation for every module.
