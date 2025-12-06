# VulkanWrapper

[![CI Tests](https://github.com/qnope/VulkanWrapper/actions/workflows/tests.yml/badge.svg)](https://github.com/qnope/VulkanWrapper/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Vulkan 1.3](https://img.shields.io/badge/Vulkan-1.3-red.svg)](https://www.vulkan.org/)

A modern C++23 library providing high-level abstractions over the Vulkan graphics API. VulkanWrapper simplifies Vulkan development while maintaining full access to advanced features like hardware ray tracing and dynamic rendering.

## Features

- **Modern Vulkan 1.3** - Dynamic rendering, Synchronization 2.0, hardware ray tracing
- **RAII Resource Management** - Automatic lifetime management with zero manual cleanup
- **Type-Safe Buffers** - Compile-time validated `Buffer<T, HostVisible, Usage>` templates
- **Automatic Synchronization** - ResourceTracker generates optimal barriers automatically
- **Builder Pattern APIs** - Fluent, readable initialization for all components
- **Ray Tracing Support** - BLAS/TLAS acceleration structures and ray tracing pipelines
- **3D Model Loading** - Assimp integration for OBJ, GLTF, FBX, and more
- **Cross-Platform Windowing** - SDL3 integration with Vulkan surface support

## Requirements

- **Compiler**: C++23 compatible (Clang 20+, GCC 14+, MSVC 2022+)
- **CMake**: 3.25 or higher
- **Vulkan SDK**: 1.3 or higher
- **vcpkg**: For dependency management

### Dependencies (managed via vcpkg)

- SDL3 (with Vulkan support)
- SDL3-image
- GLM
- Assimp
- Vulkan-Memory-Allocator
- GoogleTest

## Building

```bash
# Clone the repository
git clone https://github.com/qnope/VulkanWrapper.git
cd VulkanWrapper

# Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the library
cmake --build build

# Run tests
ctest --test-dir build --verbose
```

## Quick Start

### Instance and Device Creation

```cpp
#include <VulkanWrapper/Vulkan/InstanceBuilder.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Memory/AllocatorBuilder.h>

// Create Vulkan instance
auto instance = vw::InstanceBuilder()
    .addPortability()
    .setApiVersion(vw::ApiVersion::e13)
    .build();

// Find and create a suitable GPU device
auto device = instance.findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// Create memory allocator
auto allocator = vw::AllocatorBuilder(device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

### Type-Safe Buffers

```cpp
#include <VulkanWrapper/Memory/Buffer.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Host-visible vertex buffer (CPU-accessible)
vw::Buffer<Vertex, true, vk::BufferUsageFlagBits::eVertexBuffer> hostBuffer;

// Device-local index buffer (GPU-only, high performance)
vw::Buffer<uint32_t, false, vk::BufferUsageFlagBits::eIndexBuffer> deviceBuffer;

// Type-safe: copy_from_host() only available on host-visible buffers
hostBuffer.copy_from_host(vertices);  // OK
// deviceBuffer.copy_from_host(indices);  // Compile error!
```

### Automatic Resource Synchronization

```cpp
#include <VulkanWrapper/Synchronization/ResourceTracker.h>

vw::ResourceTracker tracker;

// Track resource states - barriers generated automatically
tracker.track_buffer_state(buffer, interval,
    vk::PipelineStageFlagBits2::eTransfer,
    vk::AccessFlagBits2::eTransferWrite);

tracker.track_buffer_state(buffer, interval,
    vk::PipelineStageFlagBits2::eVertexShader,
    vk::AccessFlagBits2::eShaderRead);

// Generate optimal barriers
tracker.flush(commandBuffer);
```

### Graphics Pipeline

```cpp
#include <VulkanWrapper/Pipeline/GraphicsPipelineBuilder.h>

auto pipeline = vw::GraphicsPipelineBuilder(device)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertexShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragmentShader)
    .set_vertex_input<Vertex>()
    .set_primitive_topology(vk::PrimitiveTopology::eTriangleList)
    .set_color_attachment_format(swapchain.format())
    .set_depth_attachment_format(vk::Format::eD32Sfloat)
    .build();
```

## Project Structure

```
VulkanWrapper/
├── VulkanWrapper/
│   ├── include/VulkanWrapper/
│   │   ├── Vulkan/          # Instance, Device, Queue, Swapchain
│   │   ├── Memory/          # Allocator, Buffers, Staging
│   │   ├── Image/           # Images, Views, Samplers, Loaders
│   │   ├── Command/         # CommandPool, CommandBufferRecorder
│   │   ├── Pipeline/        # Graphics/Compute pipelines
│   │   ├── Descriptors/     # Descriptor sets and layouts
│   │   ├── RenderPass/      # Dynamic rendering abstractions
│   │   ├── Model/           # Mesh loading, Materials
│   │   ├── RayTracing/      # BLAS, TLAS, RT pipelines
│   │   ├── Synchronization/ # Fences, Semaphores, ResourceTracker
│   │   └── Window/          # SDL3 windowing
│   ├── src/                 # Implementation files
│   └── tests/               # Unit tests
├── examples/
│   ├── Advanced/            # Deferred rendering + ray tracing demo
│   └── Application/         # Base application template
└── Models/                  # 3D assets (Sponza scene)
```

## Modules

| Module | Description |
|--------|-------------|
| **Vulkan** | Core GPU initialization with builder patterns |
| **Memory** | Type-safe buffers, staging, and VMA integration |
| **Image** | Textures, image views, samplers, and mipmaps |
| **Command** | RAII command buffer recording |
| **Pipeline** | Graphics and ray tracing pipeline builders |
| **Descriptors** | Descriptor set management with fluent API |
| **RenderPass** | Vulkan 1.3 dynamic rendering abstractions |
| **Model** | 3D model loading via Assimp with material system |
| **RayTracing** | Hardware ray tracing with BLAS/TLAS |
| **Synchronization** | Automatic barrier generation via ResourceTracker |
| **Window** | Cross-platform windowing with SDL3 |

## Examples

### Advanced Example

The `examples/Advanced/` directory contains a sophisticated demo featuring:

- **Deferred Rendering Pipeline** with G-buffer generation
- **Hardware Ray Tracing** for lighting calculations
- **Multi-Pass Rendering**: Z-Pass, G-Buffer, Sun Light, Sky, Tone Mapping
- **Complex Scene** rendering with the Sponza model

```bash
# Build and run the advanced example
cmake --build build --target Advanced
./build/examples/Advanced/Advanced
```

## Testing

```bash
# Run all tests
ctest --test-dir build

# Run specific test suite
./build/VulkanWrapper/tests/MemoryTests
./build/VulkanWrapper/tests/ResourceTrackerTests

# Verbose output
ctest --test-dir build --verbose
```

### Test Suites

- Memory allocation and buffer management
- Image creation and views
- Instance and device creation
- Resource state tracking and synchronization
- Interval set operations

## Shader Compilation

Use the `VwCompileShader` CMake function to compile GLSL shaders:

```cmake
VwCompileShader(your_target "shader.vert" "shader.vert.spv")
VwCompileShader(your_target "shader.frag" "shader.frag.spv")
```

## Modern C++ Features

VulkanWrapper leverages C++23 features including:

- `std::span` for safe array access
- Ranges library for algorithms
- Concepts for compile-time constraints
- `std::source_location` for error reporting
- Structured bindings and template parameter deduction

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**Antoine Morrier** - [qnope](https://github.com/qnope)

## Acknowledgments

- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) for C++ bindings
- [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for memory allocation
- [Assimp](https://github.com/assimp/assimp) for model loading
- [SDL3](https://github.com/libsdl-org/SDL) for windowing and input
