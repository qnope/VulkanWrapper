# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Building the Project
```bash
# Configure with CMake (creates build directory)
cmake -B build-Clang20Debug -DCMAKE_BUILD_TYPE=Debug

# Build the entire project
cmake --build build-Clang20Debug

# Build specific target
cmake --build build-Clang20Debug --target VulkanWrapperCoreLibrary
```

### Running Tests
```bash
# Run all tests
ctest --test-dir build-Clang20Debug

# Run specific test suite
./build-Clang20Debug/VulkanWrapper/tests/MemoryTests
./build-Clang20Debug/VulkanWrapper/tests/ImageTests

# Run tests with verbose output
ctest --test-dir build-Clang20Debug --verbose
```

If test with screenshot asked
```bash
cd directory_of_the_executable
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib/libvulkan.dylib:$DYLD_LIBRARY_PATH ./Main
```

### Debugging tests
You can use lldb to debug tests


## Project Architecture

VulkanWrapper is a modern C++23 library providing high-level abstractions over the Vulkan graphics API. The library follows a layered architecture with extensive use of RAII, builder patterns, and compile-time type safety.

### Core Design Principles
- **Builder Pattern**: All major objects use fluent builder APIs (InstanceBuilder, DeviceFinder, AllocatorBuilder, GraphicsPipelineBuilder, etc.)
- **RAII**: Automatic lifetime management through ObjectWithHandle wrapper and unique Vulkan handles
- **Type Safety**: Template-based buffers `Buffer<T, HostVisible, Usage>` with compile-time usage validation
- **Automatic Resource Tracking**: ResourceTracker automatically generates Vulkan barriers based on resource state transitions
- **Modern Vulkan**: Uses Vulkan 1.3 features (dynamic rendering, synchronization2, ray tracing). It uses Vulkan HPP

### Module Organization

**Vulkan/** - Core Vulkan initialization and device management
- Instance, Device, PhysicalDevice: Core Vulkan objects with builder patterns
- DeviceFinder: Fluent API for GPU selection based on queues, extensions, and features
- Queue, PresentQueue: Command submission
- Swapchain: Presentation chain management

**Memory/** - Memory allocation and buffer management
- Allocator: Wraps VMA (Vulkan Memory Allocator) for all allocations
- Buffer<T, HostVisible, Usage>: Typed buffer template with compile-time visibility flags
- BufferList: Dynamic buffer allocation with automatic 16MB paging
- StagingBufferManager: Queues and batches CPU-to-GPU transfers
- UniformBufferAllocator: Specialized allocator for uniform buffers with alignment
- Interval/IntervalSet: Track memory ranges for fine-grained barrier management

**Image/** - Texture and image handling
- Image, ImageView, Sampler: Core image objects
- ImageLoader: Loads textures from disk (via SDL_image)
- Mipmap: Automatic mipmap generation

**Command/** - Command buffer recording
- CommandPool: Command buffer allocation with builder
- CommandBufferRecorder: RAII-style recording (begin in constructor, end in destructor)

**Pipeline/** - Graphics pipeline management
- Pipeline: Graphics pipeline with extensive builder support
- PipelineLayout: Descriptor set layouts and push constants
- ShaderModule: Shader loading from SPIR-V
- MeshRenderer: High-level rendering abstraction with material-based pipeline selection

**Descriptors/** - Descriptor management
- DescriptorSetLayout: Layout with fluent binding API
- DescriptorPool, DescriptorSet: Allocation and management
- DescriptorAllocator: Tracks descriptor updates and resource states

**RenderPass/** - Rendering abstraction (uses Vulkan 1.3 dynamic rendering)
- Rendering: Sequence of subpasses
- Subpass: Abstract interface for render passes (no VkRenderPass objects)
- ScreenSpacePass: Full-screen rendering utilities

**Model/** - 3D model loading and materials
- Mesh, MeshManager: Mesh data and management
- Importer: Loads models from files (uses Assimp)
- Material system: Polymorphic material managers (ColoredMaterialManager, TexturedMaterialManager)
- MaterialManagerMap: Maps material types to descriptor pools and managers

**RayTracing/** - Ray tracing support
- BottomLevelAccelerationStructure (BLAS): Geometry acceleration structures
- TopLevelAccelerationStructure (TLAS): Instance-level acceleration structures
- RayTracingPipeline: Ray tracing pipeline with builder
- ShaderBindingTable: SBT management

**Synchronization/** - Synchronization and barriers
- Fence, Semaphore: Standard synchronization primitives
- ResourceTracker: Automatic barrier generation based on tracked resource states
  - Tracks images, buffers, and acceleration structures
  - Uses IntervalSets for sub-buffer state tracking
  - Generates minimal barrier sets on flush

**Window/** - SDL2 windowing
- SDL_Initializer: RAII wrapper for SDL initialization
- Window: Window creation with builder pattern

### Key Architectural Patterns

**Typical Initialization Flow:**
```cpp
// Instance creation
auto instance = InstanceBuilder()
    .addPortability()
    .setApiVersion(ApiVersion::e13)
    .build();

// Device selection and creation
auto device = instance.findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// Allocator creation
auto allocator = AllocatorBuilder(device)
    .set_flags(VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT)
    .build();
```

**Resource Tracking Pattern:**
The ResourceTracker automatically manages Vulkan barriers:
```cpp
// Track current state of resources
tracker.track_buffer_state(buffer, interval, vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite);

// Request new state
tracker.track_buffer_state(buffer, interval, vk::PipelineStageFlagBits2::eVertexShader, vk::AccessFlagBits2::eShaderRead);

// Flush generates minimal barrier set
tracker.flush(cmd);
```

**Buffer Type Safety:**
Buffer types are validated at compile-time using template parameters:
```cpp
Buffer<Vertex, true, vk::BufferUsageFlagBits::eVertexBuffer> hostVisibleVertexBuffer;
Buffer<uint32_t, false, vk::BufferUsageFlagBits::eIndexBuffer> deviceLocalIndexBuffer;

// Compile error if usage doesn't match:
hostVisibleVertexBuffer.copy_from_host(...); // OK - HostVisible=true
deviceLocalIndexBuffer.copy_from_host(...);  // Compile error - HostVisible=false
```

### Dependencies

**External Libraries (vcpkg.json):**
- SDL3: Windowing and input (with Vulkan support)
- SDL3-image: Image loading (PNG format)
- GLM: Math library (vectors, matrices)
- Assimp: 3D model loading
- Vulkan-Memory-Allocator: Memory allocation
- GTest: Unit testing

**Internal Dependencies:**
All core functionality depends on Device and Allocator. ResourceTracker sits at the top of the dependency chain, tracking all resource types (buffers, images, acceleration structures).

### C++23 Features Used

- std::span for safe array access
- Ranges library (std::ranges::sort, std::ranges::views)
- Concepts for compile-time constraints
- std::source_location for error reporting
- Structured bindings
- Template parameter deduction
- Extensive use of algorithms

### Shader Compilation

Use the VwCompileShader CMake function to compile shaders written in GLSL:
```cmake
VwCompileShader(target_name "shader.vert" "output.vert.spv")
```

Shaders are compiled with glslangValidator and output to the target's runtime directory.

### Testing Strategy

Test utilities are in VulkanWrapper/tests/utils/create_gpu.cpp which provides a singleton Device instance.

### Workflow
1. Explore the code
2. Write the units tests if possible
3. compile (if possible) and execute the tests written to see that they fail
4. Make the implementation
5. Execute the tests:
    1. If they don't pass: go back on 4)
6. Compile everything (examples also)