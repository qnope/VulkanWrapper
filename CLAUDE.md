# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VulkanWrapper is a modern C++23 library providing high-level abstractions over the Vulkan graphics API. It emphasizes:
- **Type safety** through templates and concepts
- **RAII** for automatic resource management
- **Builder patterns** for fluent object construction
- **Modern Vulkan 1.3** features (dynamic rendering, synchronization2, ray tracing)

## Build Commands

### Prerequisites
Dependencies are managed via vcpkg (see `vcpkg.json`):
- SDL3 (with Vulkan support)
- SDL3-image (PNG format)
- GLM (math library)
- Assimp (3D model loading)
- Vulkan + validation layers
- glslang + shaderc (shader compilation)
- Vulkan Memory Allocator
- GTest

### Building the Project
```bash
# Configure with CMake (Debug)
cmake -B build-Clang20Debug -DCMAKE_BUILD_TYPE=Debug

# Configure with CMake (Release)
cmake -B build-Clang20Release -DCMAKE_BUILD_TYPE=Release

# Build entire project
cmake --build build-Clang20Debug

# Build specific target
cmake --build build-Clang20Debug --target VulkanWrapperCoreLibrary

# Build with code coverage (Clang only)
cmake -B build-Coverage -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build-Coverage
```

### Running Tests
```bash
# Run all tests via CTest (with library path for macOS)
cd build-Clang20Debug && DYLD_LIBRARY_PATH=vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ctest

# Run with verbose output
cd build-Clang20Debug && DYLD_LIBRARY_PATH=vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ctest --verbose

# Run specific test executables directly
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./MemoryTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./ImageTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./VulkanTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./RayTracingTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./UtilsTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./ShaderTests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./RenderPassTests
```

### Running Examples
```bash
cd build-Clang20Debug/examples/Advanced
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./Main
```

### Debugging with LLDB
```bash
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH lldb ./MemoryTests
```

## Project Architecture

### Directory Structure
```
VulkanWrapper/
├── VulkanWrapper/           # Main library
│   ├── include/VulkanWrapper/  # Public headers
│   │   ├── Command/         # Command buffer recording
│   │   ├── Descriptors/     # Descriptor sets and layouts
│   │   ├── Image/           # Images, views, samplers
│   │   ├── Memory/          # Allocator, buffers, resource tracking
│   │   ├── Model/           # Mesh loading and materials
│   │   ├── Pipeline/        # Graphics/compute pipelines
│   │   ├── RayTracing/      # RT acceleration structures
│   │   ├── RenderPass/      # Subpass and screen-space passes
│   │   ├── Shader/          # Shader compilation
│   │   ├── Synchronization/ # Fences, semaphores, barriers
│   │   ├── Utils/           # Error handling, helpers
│   │   ├── Vulkan/          # Instance, device, queues
│   │   └── Window/          # SDL3 window management
│   ├── src/VulkanWrapper/   # Implementation files
│   ├── tests/               # Unit tests (GTest)
│   └── Shaders/             # Common shaders (fullscreen.vert)
├── examples/
│   ├── Application/         # Base application framework
│   └── Advanced/            # Deferred rendering demo
└── Images/                  # Test images
```

### Core Design Principles

1. **Builder Pattern**: All major objects use fluent builder APIs
   - `InstanceBuilder`, `DeviceFinder`, `AllocatorBuilder`
   - `GraphicsPipelineBuilder`, `CommandPoolBuilder`
   - `ImageViewBuilder`, `SamplerBuilder`, `SemaphoreBuilder`

2. **RAII & Ownership**:
   - `ObjectWithHandle<T>`: Base class for Vulkan handle wrappers
   - `ObjectWithUniqueHandle<T>`: For unique Vulkan handles (automatic destruction)
   - `std::shared_ptr` for shared ownership across components
   - `std::unique_ptr` for exclusive ownership (buffers, internal data)

3. **Type-Safe Buffers**:
   ```cpp
   Buffer<T, HostVisible, Usage>
   // T: Element type
   // HostVisible: true = CPU-accessible, false = GPU-only
   // Usage: VkBufferUsageFlags (compile-time validated)

   // Predefined aliases
   using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;
   ```

4. **Automatic Resource Tracking** (`ResourceTracker`):
   - Tracks state of images, buffers, and acceleration structures
   - Uses `IntervalSet` for fine-grained sub-buffer state tracking
   - Generates minimal barrier sets on `flush()`
   ```cpp
   tracker.track(ImageState{...});   // Record current state
   tracker.request(ImageState{...}); // Request new state
   tracker.flush(cmd);               // Generate barriers
   ```

### Module Details

**Vulkan/** - Core initialization
- `Instance`: Vulkan instance with validation layers support
- `DeviceFinder`: Fluent GPU selection (queues, extensions, features)
- `Device`: Logical device with queue access
- `Swapchain`: Presentation chain management

**Memory/** - Resource allocation
- `Allocator`: Wraps VMA for all allocations
- `Buffer<T, HostVisible, Usage>`: Typed buffer with compile-time safety
- `BufferList`: Dynamic allocation with 16MB paging
- `StagingBufferManager`: Batched CPU-to-GPU transfers
- `UniformBufferAllocator`: Aligned uniform buffer allocation
- `Interval`/`IntervalSet`: Memory range tracking

**Image/** - Texture handling
- `Image`: 2D/3D images with automatic format handling
- `ImageView`: Views with subresource ranges
- `Sampler`: Filtering and addressing modes
- `ImageLoader`: Load from disk (via SDL3_image)
- `Mipmap`: Automatic mipmap generation

**Command/** - Recording
- `CommandPool`: Command buffer allocation
- `CommandBufferRecorder`: RAII recording (begin in ctor, end in dtor)

**Pipeline/** - Shaders and state
- `Pipeline`: Graphics pipeline with builder
- `PipelineLayout`: Descriptor set layouts + push constants
- `ShaderModule`: SPIR-V loading
- `MeshRenderer`: Material-based rendering abstraction

**Descriptors/** - Binding resources
- `DescriptorSetLayout`: Layout with fluent binding API
- `DescriptorPool`, `DescriptorSet`: Allocation and management
- `DescriptorAllocator`: Tracks updates and states
- `Vertex`: Concept + implementations for vertex types

**RenderPass/** - Rendering abstraction (Vulkan 1.3 dynamic rendering)
- `Subpass<SlotEnum>`: Base class with lazy image allocation
- `ScreenSpacePass<SlotEnum>`: Full-screen post-processing helpers
- `create_screen_space_pipeline()`: Pipeline factory for screen-space passes

**Model/** - 3D assets
- `Mesh`, `MeshManager`: Mesh data loading (via Assimp)
- `Material`: Polymorphic material system
- `MaterialManagerMap`: Material type to descriptor pool mapping

**RayTracing/** - Hardware RT
- `BottomLevelAccelerationStructure` (BLAS)
- `TopLevelAccelerationStructure` (TLAS)
- `RayTracedScene`: Scene management with instance transforms
- `RayTracingPipeline` + `ShaderBindingTable`

**Synchronization/** - Barriers and sync
- `Fence`, `Semaphore`: Primitives
- `ResourceTracker`: Automatic barrier generation

**Window/** - Windowing
- `SDL_Initializer`: RAII SDL init
- `Window`: Window with builder pattern

### Error Handling

The library uses a hierarchy of exceptions (all inherit from `vw::Exception`):
```cpp
vw::Exception           // Base - message + std::source_location
├── VulkanException     // vk::Result errors
├── SDLException        // SDL errors (auto-captures SDL_GetError())
├── VMAException        // VMA allocation errors
├── FileException       // File I/O errors
├── AssimpException     // Model loading errors
├── ShaderCompilationException  // GLSL compilation errors
└── LogicException      // Logic/state errors (out_of_range, invalid_state, null_pointer)
```

Helper functions for result checking:
```cpp
check_vk(result, "context");           // Throws VulkanException if !eSuccess
check_vk(result_pair, "context");      // For std::pair<vk::Result, T>
check_vk(result_value, "context");     // For vk::ResultValue<T>
check_sdl(success, "context");         // Throws SDLException if false
check_sdl(ptr, "context");             // Throws SDLException if nullptr
check_vma(result, "context");          // Throws VMAException if !VK_SUCCESS
```

### Key Patterns

**Typical Initialization:**
```cpp
auto instance = InstanceBuilder()
    .setDebug()                        // Enable validation layers
    .addPortability()                  // For MoltenVK on macOS
    .setApiVersion(ApiVersion::e13)
    .build();

auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

auto allocator = AllocatorBuilder(instance, device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**Resource Tracking:**
```cpp
vw::Transfer transfer;  // Contains a ResourceTracker

// Track current state
transfer.resourceTracker().track(vw::Barrier::ImageState{
    .image = image->handle(),
    .subresourceRange = image->full_range(),
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});

// Request new state
transfer.resourceTracker().request(vw::Barrier::ImageState{...});

// Generate and record barriers
transfer.resourceTracker().flush(cmd);

// High-level operations
transfer.blit(cmd, srcImage, dstImage);
transfer.saveToFile(cmd, allocator, queue, image, "output.png");
```

**Screen-Space Pass Pattern:**
```cpp
class MyPass : public ScreenSpacePass<MySlots> {
public:
    MyPass(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator)
        : ScreenSpacePass(device, allocator) {
        // Create pipeline, descriptor pool, sampler...
        m_sampler = create_default_sampler();
    }

    void execute(vk::CommandBuffer cmd, /*...*/) {
        auto& cached = get_or_create_image(MySlots::Output, width, height, frameIndex, format, usage);

        vk::RenderingAttachmentInfo color_attachment = /*...*/;
        render_fullscreen(cmd, extent, color_attachment, nullptr, *m_pipeline, m_descriptor_set, push_constants, sizeof(push_constants));
    }
};
```

**Command Buffer Recording:**
```cpp
auto commandPool = vw::CommandPoolBuilder(device).build();
auto commandBuffers = commandPool.allocate(count);

{
    vw::CommandBufferRecorder recorder(commandBuffers[0]); // Begins recording
    // Record commands...
} // Ends recording automatically
```

### Shader Compilation

Use `VwCompileShader` CMake function:
```cmake
VwCompileShader(target_name "shader.vert" "output.vert.spv")
VwCompileShader(target_name "shader.frag" "output.frag.spv")

# With extra args (e.g., for ray tracing)
VwCompileShader(target_name "raygen.rgen" "raygen.rgen.spv" --target-env vulkan1.2)
```

Shaders are compiled with `glslangValidator` and output to target's runtime directory.

Runtime shader compilation is also available via `ShaderCompiler` class.

### Testing

**Test Structure:**
- Tests in `VulkanWrapper/tests/` organized by module
- `TestUtils` library provides singleton GPU instance via `create_gpu()`
- GTest framework with `gtest_discover_tests()`

**Test Utilities (`utils/create_gpu.hpp`):**
```cpp
namespace vw::tests {
    struct GPU {
        std::shared_ptr<Instance> instance;
        std::shared_ptr<Device> device;
        std::shared_ptr<Allocator> allocator;
        vw::Queue& queue();
    };
    GPU& create_gpu();  // Singleton, intentionally leaked to avoid destruction order issues
}
```

**Adding a New Test:**
1. Create test file in appropriate subdirectory (e.g., `Memory/NewTests.cpp`)
2. Add to test executable in `VulkanWrapper/tests/CMakeLists.txt`
3. Link `TestUtils`, `VulkanWrapperCoreLibrary`, `GTest::gtest`, `GTest::gtest_main`
4. Call `gtest_discover_tests(NewTests)`

### C++23 Features Used

- `std::span` for safe array views
- Ranges (`std::ranges::sort`, `std::views`)
- Concepts for compile-time constraints
- `std::source_location` for error reporting
- Structured bindings
- `auto operator<=>(...)` for default comparisons
- `requires` clauses for conditional member functions
- `static consteval` for compile-time buffer validation

### Code Style Conventions

- **Namespace**: `vw` (main), `vw::Barrier`, `vw::Model`, `vw::rt` (ray tracing)
- **Headers**: `.h` extension, `#pragma once`
- **Naming**: `snake_case` for functions/variables, `PascalCase` for types
- **Strong types**: `Width`, `Height`, `Depth`, `MipLevel` for type-safe dimensions
- **Vulkan-Hpp**: Always use `vk::` types, not `Vk` C types
- **Dynamic dispatcher**: Configured via `3rd_party.h` (no global dispatcher)
- **GLM config**: `GLM_FORCE_RADIANS`, `GLM_FORCE_DEPTH_ZERO_TO_ONE`

### Development Workflow

1. **Explore** the codebase to understand existing patterns
2. **Write unit tests** first (TDD approach)
3. **Compile and run tests** to verify they fail
4. **Implement** the feature/fix
5. **Run tests** - iterate until passing
6. **Build everything** including examples:
   ```bash
   cmake --build build-Clang20Debug
   ```
7. **Run examples** to verify integration:
   ```bash
   cd build-Clang20Debug/examples/Advanced
   DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./Main
   ```

### Debugging Tips

- Enable validation layers via `InstanceBuilder().setDebug()`
- Use LLDB with `DYLD_LIBRARY_PATH` set for dynamic libraries
- Check `vw::Exception::location()` for source location of errors
- Use `device->wait_idle()` before debugging GPU state
- Vulkan validation messages appear in stderr

### Important Files

- `VulkanWrapper/include/VulkanWrapper/3rd_party.h`: Precompiled header with common includes and Vulkan-Hpp config
- `VulkanWrapper/include/VulkanWrapper/fwd.h`: Forward declarations
- `VulkanWrapper/include/VulkanWrapper/Utils/Error.h`: Exception hierarchy
- `VulkanWrapper/tests/utils/create_gpu.hpp`: Test GPU singleton
- `examples/Application/Application.h`: Base application class for examples
