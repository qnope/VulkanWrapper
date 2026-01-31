---
sidebar_position: 1
---

# Introduction

**VulkanWrapper** is a modern C++23 library that provides high-level abstractions over the Vulkan graphics API. It aims to make Vulkan development more accessible while maintaining the full power and flexibility of the underlying API.

## Why VulkanWrapper?

Vulkan is a powerful, low-level graphics API that provides explicit control over GPU resources. However, this power comes with significant complexity:

- Manual resource lifetime management
- Verbose object creation with many parameters
- Complex synchronization requirements
- Easy to make mistakes with validation layers off

VulkanWrapper addresses these challenges by providing:

### Type Safety

Modern C++23 features ensure compile-time validation of your code:

```cpp
// Strong types prevent parameter mistakes
void resize(Width width, Height height);  // Can't accidentally swap

// Compile-time buffer validation
Buffer<Vertex, false, VertexBufferUsage> buffer;
static_assert(buffer.does_support(vk::BufferUsageFlagBits::eVertexBuffer));
```

### RAII Resource Management

All Vulkan resources are automatically cleaned up when they go out of scope:

```cpp
{
    auto buffer = allocator->allocate<VertexBuffer>(1000);
    // Use buffer...
}  // Buffer automatically freed here
```

### Fluent Builder APIs

Complex objects are constructed through intuitive, chainable builders:

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragmentShader)
    .add_vertex_binding<FullVertex3D>()
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .set_depth_format(vk::Format::eD32Sfloat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .build();
```

### Automatic Synchronization

The `ResourceTracker` system handles memory barriers automatically:

```cpp
ResourceTracker tracker;
tracker.track(currentImageState);
tracker.request(newImageState);
tracker.flush(commandBuffer);  // Generates optimal barriers
```

## Features

- **Vulkan 1.3** - Built for modern Vulkan with dynamic rendering and synchronization2
- **Ray Tracing** - Full hardware ray tracing support with BLAS/TLAS management
- **PBR Pipeline** - Physical-based rendering with atmospheric scattering and HDR
- **Model Loading** - Assimp integration for 3D model import
- **Shader Compilation** - Runtime GLSL to SPIR-V compilation

## Requirements

- C++23 compatible compiler (Clang 18+, GCC 14+)
- Vulkan 1.3 capable driver
- CMake 3.25+
- vcpkg for dependency management

## Project Structure

```
VulkanWrapper/
├── VulkanWrapper/           # Main library
│   ├── include/VulkanWrapper/  # Public headers
│   │   ├── Command/         # Command buffer recording
│   │   ├── Descriptors/     # Descriptor sets and layouts
│   │   ├── Image/           # Images, views, samplers
│   │   ├── Memory/          # Allocator, buffers
│   │   ├── Model/           # Mesh, materials, scenes
│   │   ├── Pipeline/        # Graphics/compute pipelines
│   │   ├── RayTracing/      # RT acceleration structures
│   │   ├── RenderPass/      # Render pass abstractions
│   │   ├── Shader/          # Shader compilation
│   │   ├── Synchronization/ # Fences, semaphores, barriers
│   │   ├── Vulkan/          # Instance, device, queues
│   │   └── Window/          # SDL3 window management
│   ├── src/                 # Implementation files
│   └── tests/               # Unit tests
└── examples/                # Example applications
    ├── Application/         # Base application framework
    └── Advanced/            # Deferred rendering demo
```

## Next Steps

- Follow the [Getting Started](./getting-started) guide to set up your first project
- Learn about [Core Concepts](./category/core-concepts) like the builder pattern and RAII
- Explore the [API Reference](./category/api-reference) for detailed documentation
