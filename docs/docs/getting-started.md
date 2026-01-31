---
sidebar_position: 2
---

# Getting Started

This guide will help you set up VulkanWrapper and create your first Vulkan application.

## Prerequisites

Before you begin, ensure you have the following installed:

- **C++23 Compiler**: Clang 18+ or GCC 14+
- **CMake**: Version 3.25 or higher
- **vcpkg**: For dependency management
- **Vulkan SDK**: For development headers and validation layers

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/qnope/VulkanWrapper.git
cd VulkanWrapper
```

### 2. Configure with CMake

VulkanWrapper uses vcpkg for dependency management. The dependencies are defined in `vcpkg.json`:

```json
{
  "dependencies": [
    {"name": "sdl3", "features": ["vulkan"]},
    {"name": "sdl3-image", "features": ["png"]},
    "glm",
    "assimp",
    "vulkan",
    "vulkan-validationlayers",
    {"name": "glslang", "features": ["tools"]},
    "shaderc",
    "vulkan-memory-allocator",
    "gtest"
  ]
}
```

Configure the project:

```bash
# Debug build
cmake -B build-Debug -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake -B build-Release -DCMAKE_BUILD_TYPE=Release
```

### 3. Build the Project

```bash
# Build everything
cmake --build build-Debug

# Build a specific target
cmake --build build-Debug --target VulkanWrapperCoreLibrary
```

### 4. Run Tests

```bash
cd build-Debug && ctest --verbose
```

## Your First Application

Here's a minimal example that initializes Vulkan with VulkanWrapper:

```cpp
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Window/Window.h>

int main() {
    // Initialize SDL
    vw::SDL_Initializer sdl;

    // Create a window
    auto window = vw::WindowBuilder()
        .setWidth(1280)
        .setHeight(720)
        .setTitle("My First VulkanWrapper App")
        .build();

    // Create Vulkan instance
    auto instance = vw::InstanceBuilder()
        .setDebug()  // Enable validation layers
        .setApiVersion(vw::ApiVersion::e13)
        .addExtensions(window->vulkan_extensions())
        .build();

    // Find a suitable GPU
    auto device = instance->findGpu()
        .requireGraphicsQueue()
        .requirePresentQueue(window->create_surface(instance))
        .requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .find()
        .build();

    // Create memory allocator
    auto allocator = vw::AllocatorBuilder()
        .setInstance(instance)
        .setDevice(device)
        .build();

    // Your rendering code here...

    return 0;
}
```

## Creating Resources

### Buffers

VulkanWrapper provides type-safe buffer abstractions:

```cpp
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Memory/BufferUsage.h>

// Define your vertex type
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Create a vertex buffer (GPU-only)
using VertexBuffer = vw::Buffer<Vertex, false, vw::VertexBufferUsage>;
auto vertexBuffer = allocator->allocate<VertexBuffer>(vertices.size());

// Create a staging buffer (CPU-visible)
using StagingBuffer = vw::Buffer<Vertex, true, vw::TransferSrcUsage>;
auto stagingBuffer = allocator->allocate<StagingBuffer>(vertices.size());

// Write data to staging buffer
stagingBuffer->write(vertices, 0);
```

### Images

```cpp
#include <VulkanWrapper/Image/Image.h>
#include <VulkanWrapper/Image/ImageView.h>
#include <VulkanWrapper/Image/Sampler.h>

// Create an image
auto image = vw::ImageBuilder()
    .setDevice(device)
    .setAllocator(allocator)
    .setFormat(vk::Format::eR8G8B8A8Srgb)
    .setExtent(vw::Width{1024}, vw::Height{1024})
    .setUsage(vk::ImageUsageFlagBits::eSampled |
              vk::ImageUsageFlagBits::eTransferDst)
    .build();

// Create an image view
auto imageView = vw::ImageViewBuilder()
    .setDevice(device)
    .setImage(image)
    .build();

// Create a sampler
auto sampler = vw::SamplerBuilder()
    .setDevice(device)
    .setFilter(vk::Filter::eLinear)
    .setAddressMode(vk::SamplerAddressMode::eRepeat)
    .build();
```

### Graphics Pipeline

```cpp
#include <VulkanWrapper/Pipeline/Pipeline.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>

// Compile shaders
auto vertShader = vw::ShaderCompiler::compile(
    device, "shader.vert", vk::ShaderStageFlagBits::eVertex);
auto fragShader = vw::ShaderCompiler::compile(
    device, "shader.frag", vk::ShaderStageFlagBits::eFragment);

// Create pipeline layout
auto layout = vw::PipelineLayoutBuilder()
    .setDevice(device)
    .addDescriptorSetLayout(descriptorLayout)
    .addPushConstantRange<PushConstants>(vk::ShaderStageFlagBits::eVertex)
    .build();

// Build the pipeline
auto pipeline = vw::GraphicsPipelineBuilder(device, layout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
    .add_vertex_binding<Vertex>()
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .with_dynamic_viewport_scissor()
    .build();
```

## Running the Examples

The repository includes example applications demonstrating various features:

```bash
# Build and run the advanced deferred rendering example
cd build-Debug/examples/Advanced
./Main
```

## Next Steps

- Learn about [Core Concepts](./category/core-concepts) to understand the library's design
- Explore the [Modules](./category/modules) documentation for detailed API information
- Check out the [Tutorials](./category/tutorials) for step-by-step guides
