---
sidebar_position: 2
---

# Getting Started

This guide walks you through setting up VulkanWrapper, building it from source, and running your first Vulkan program. No prior Vulkan experience is required -- if you are new to Vulkan, consider reading the [Vulkan Primer](./vulkan-primer) after completing this page.

## Prerequisites

You will need the following tools installed on your system:

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| **C++23 compiler** | Clang 20+ | Compiles the library (uses concepts, ranges, `std::source_location`) |
| **CMake** | 3.25+ | Build system generator |
| **vcpkg** | Latest | C++ package manager for all dependencies |
| **Vulkan SDK** | 1.3+ | Vulkan headers, validation layers, and loader |
| **Ninja** (recommended) | Any | Fast parallel build system |

### Installing Prerequisites

**macOS (Homebrew):**

```bash
brew install llvm cmake ninja
# Install vcpkg if not already set up:
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg
```

**Linux (Ubuntu/Debian):**

```bash
# Install Vulkan SDK from https://vulkan.lunarg.com/sdk/home
sudo apt install cmake ninja-build
# Install vcpkg:
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg
```

Make sure the `VCPKG_ROOT` environment variable is set and points to your vcpkg installation directory.

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/qnope/VulkanWrapper.git
cd VulkanWrapper
```

### 2. Configure with CMake

VulkanWrapper uses vcpkg to manage all its dependencies automatically. The `vcpkg.json` manifest at the repository root declares everything needed:

- **SDL3** (with Vulkan support) -- windowing and input
- **SDL3-image** (with PNG support) -- image loading and saving
- **GLM** -- mathematics (vectors, matrices, transforms)
- **Assimp** -- 3D model loading (OBJ, GLTF, FBX, and more)
- **Vulkan** + **Vulkan Validation Layers** -- Vulkan headers and debugging
- **glslang** (with tools) -- GLSL tooling
- **shaderc** -- runtime GLSL to SPIR-V compilation
- **Vulkan Memory Allocator (VMA)** -- efficient GPU memory management
- **GoogleTest** -- unit testing framework

Configure the project (vcpkg will automatically download and build dependencies on the first run -- this may take several minutes):

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
    -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -G "Ninja"
```

If you are on Linux and your system Clang is already version 20 or higher, you can omit the compiler flags:

```bash
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -G "Ninja"
```

### 3. Build the Project

```bash
# Build everything (library, tests, and examples)
cmake --build build

# Or build only a specific target
cmake --build build --target VulkanWrapperCoreLibrary
cmake --build build --target Triangle
cmake --build build --target Advanced
```

### 4. Run the Tests

```bash
cd build && ctest --stop-on-failure --output-on-failure -j8
```

On macOS, you may need to set the dynamic library path so the test executables can find vcpkg-built libraries:

```bash
cd build && DYLD_LIBRARY_PATH=vcpkg_installed/arm64-osx/lib \
    ctest --stop-on-failure --output-on-failure -j8
```

To run a specific test suite:

```bash
cd build/VulkanWrapper/tests && \
    DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib \
    ./RenderPassTests
```

To run a single test by name:

```bash
cd build/VulkanWrapper/tests && \
    DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib \
    ./RenderPassTests --gtest_filter='*IndirectLight*'
```

## Your First Application

Here is a minimal program that initializes the Vulkan instance, selects a GPU, and creates a memory allocator. This is the foundation for every VulkanWrapper application:

```cpp
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Memory/Allocator.h>

int main() {
    // 1. Create a Vulkan instance with validation layers enabled
    auto instance = vw::InstanceBuilder()
        .setApiVersion(vw::ApiVersion::e13)  // Require Vulkan 1.3
        .setDebug()                           // Enable validation layers
        .build();

    // 2. Find a GPU that supports the features we need
    auto device = instance->findGpu()
        .with_queue(vk::QueueFlagBits::eGraphics)  // Need a graphics queue
        .with_synchronization_2()                    // Modern barrier model
        .with_dynamic_rendering()                    // No VkRenderPass needed
        .build();

    // 3. Create a memory allocator (wraps Vulkan Memory Allocator)
    auto allocator = vw::AllocatorBuilder(instance, device).build();

    // 4. Use the device and allocator to create resources...
    auto image = allocator->create_image_2D(
        vw::Width{512}, vw::Height{512},
        false,  // no mipmaps
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eSampled);

    // 5. Wait for the GPU to finish before shutting down
    device->wait_idle();

    return 0;
}
```

### Adding a Window

Most applications need a window to display rendered output. VulkanWrapper integrates with SDL3 for cross-platform windowing:

```cpp
#include <VulkanWrapper/Window/SDL_Initializer.h>
#include <VulkanWrapper/Window/Window.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/DeviceFinder.h>
#include <VulkanWrapper/Vulkan/Surface.h>
#include <VulkanWrapper/Vulkan/Swapchain.h>
#include <VulkanWrapper/Memory/Allocator.h>

int main() {
    // Initialize SDL (must outlive all SDL objects)
    auto initializer = std::make_shared<vw::SDL_Initializer>();

    // Create a window
    auto window = vw::WindowBuilder(initializer)
        .with_title("My First VulkanWrapper App")
        .sized(vw::Width{1024}, vw::Height{768})
        .build();

    // Create instance with window-required extensions
    auto instance = vw::InstanceBuilder()
        .addPortability()  // Required on macOS (MoltenVK)
        .addExtensions(window.get_required_instance_extensions())
        .setApiVersion(vw::ApiVersion::e13)
        .setDebug()
        .build();

    // Create a Vulkan surface from the window
    auto surface = window.create_surface(*instance);

    // Find a GPU with presentation support
    auto device = instance->findGpu()
        .with_queue(vk::QueueFlagBits::eGraphics |
                    vk::QueueFlagBits::eCompute |
                    vk::QueueFlagBits::eTransfer)
        .with_presentation(surface.handle())  // Can present to our window
        .with_synchronization_2()
        .with_dynamic_rendering()
        .build();

    // Create allocator and swapchain
    auto allocator = vw::AllocatorBuilder(instance, device).build();
    auto swapchain = window.create_swapchain(device, surface.handle());

    // Main loop
    while (!window.is_close_requested()) {
        window.update();  // Poll events

        if (window.is_resized()) {
            // Recreate swapchain on resize
            device->wait_idle();
            swapchain = window.create_swapchain(device, surface.handle());
        }

        // Your rendering code here...
    }

    device->wait_idle();
    return 0;
}
```

This is the same pattern used by the `App` class in `examples/Application/Application.h`, which all the example programs build upon.

### Creating Buffers

VulkanWrapper provides type-safe buffer creation through template functions:

```cpp
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Create a device-local vertex buffer for 1000 vertices
auto vertexBuffer = vw::allocate_vertex_buffer<Vertex, false>(
    *allocator, 1000);

// Create a host-visible uniform buffer for camera data
auto cameraBuffer = vw::create_buffer<glm::mat4, true, vw::UniformBufferUsage>(
    *allocator, 2);  // 2 matrices: view and projection

// Write to host-visible buffers directly
std::array<glm::mat4, 2> matrices = {viewMatrix, projMatrix};
cameraBuffer.write(matrices, 0);

// Create a storage buffer for compute shader data
auto storageBuffer = vw::create_buffer<float, false, vw::StorageBufferUsage>(
    *allocator, 65536);
```

### Compiling Shaders

Shaders can be compiled from GLSL to SPIR-V at runtime:

```cpp
#include <VulkanWrapper/Shader/ShaderCompiler.h>

auto compiler = vw::ShaderCompiler()
    .add_include_path("Shaders/include");

// Compile from file (stage detected from extension: .vert, .frag, .comp, etc.)
auto vertShader = compiler.compile_file_to_module(device, "shader.vert");
auto fragShader = compiler.compile_file_to_module(device, "shader.frag");

// Or compile from a string
auto computeShader = compiler.compile_to_module(
    device,
    R"(
        #version 460
        layout(local_size_x = 256) in;
        void main() { /* ... */ }
    )",
    vk::ShaderStageFlagBits::eCompute,
    "my_compute_shader");
```

## Running the Examples

The repository includes several example applications of increasing complexity:

| Example | Description |
|---------|-------------|
| **Triangle** | Minimal triangle rendering -- the "Hello World" of Vulkan |
| **EmissiveCube** | Rotating cube with emissive materials |
| **CubeShadow** | Cube rendering with shadow mapping |
| **AmbientOcclusion** | Screen-space ambient occlusion technique |
| **Advanced** | Full deferred rendering pipeline with hardware ray tracing, atmospheric scattering, and tone mapping |

Build and run an example:

```bash
cmake --build build --target Triangle

# On macOS:
cd build/examples/Triangle && \
    DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib \
    ./Triangle

# On Linux:
cd build/examples/Triangle && ./Triangle
```

The Advanced example renders a complex scene (Sponza) and generates a `screenshot.png`:

```bash
cmake --build build --target Advanced

cd build/examples/Advanced && \
    DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib \
    ./Main
```

## Next Steps

- **[Vulkan Primer](./vulkan-primer)** -- understand the Vulkan concepts that VulkanWrapper builds upon.
- **[Core Concepts](./category/core-concepts)** -- learn about the builder pattern, RAII ownership model, type safety, and error handling in depth.
- **[Module Reference](./category/module-reference)** -- explore each module of the library (Memory, Pipeline, RayTracing, and more).
- **[Tutorial Series](./category/tutorial-series)** -- step-by-step guides that build from a triangle to global illumination.
