# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VulkanWrapper is a modern C++23 library providing high-level abstractions over the Vulkan graphics API. It emphasizes:
- **Type safety** through templates and concepts
- **RAII** for automatic resource management
- **Builder patterns** for fluent object construction
- **Modern Vulkan 1.3** features (dynamic rendering, synchronization2, ray tracing)
- **Physical-based rendering** with radiance-based calculations

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

### Vulkan Driver
The project uses **llvmpipe** as the Vulkan driver. This software driver fully supports ray tracing, so all tests and applications (including ray tracing features) work correctly with llvmpipe.

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
│   │   ├── Image/           # Images, views, samplers, CombinedImage
│   │   ├── Memory/          # Allocator, buffers, resource tracking
│   │   ├── Model/           # Mesh, materials, scene management
│   │   ├── Pipeline/        # Graphics/compute pipelines
│   │   ├── Random/          # Random sampling infrastructure
│   │   ├── RayTracing/      # RT acceleration structures
│   │   ├── RenderPass/      # Subpass, screen-space passes, sky/lighting
│   │   ├── Shader/          # Shader compilation
│   │   ├── Synchronization/ # Fences, semaphores, barriers
│   │   ├── Utils/           # Error handling, helpers
│   │   ├── Vulkan/          # Instance, device, queues
│   │   └── Window/          # SDL3 window management
│   ├── src/VulkanWrapper/   # Implementation files
│   ├── tests/               # Unit tests (GTest)
│   └── Shaders/             # Common shaders and includes
│       ├── fullscreen.vert
│       └── include/         # Shared GLSL includes
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

5. **Lazy Image Allocation**: `Subpass` base class manages image cache with automatic cleanup on dimension changes

6. **Functional Passes**: Render passes don't force image allocation; they return views, caller manages buffers

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
- `CombinedImage`: Packages ImageView + Sampler for descriptor binding

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
  - `ColoredVertex<Position>`: Position + color
  - `ColoredAndTexturedVertex<Position>`: Position + color + UV
  - `FullVertex3D`: Position + normal + tangent + bitangent + UV

**Random/** - Random sampling infrastructure
- `RandomSamplingBuffer`: Centralized random sampling for ray tracing
  - `DualRandomSample`: 4096 precomputed hemisphere samples (vec2 array)
  - `generate_hemisphere_samples()`: Reproducible samples with seed
  - `create_hemisphere_samples_buffer()`: GPU storage buffer creation
- `NoiseTexture`: 4096x4096 RG32F noise texture for Cranley-Patterson rotation

**RenderPass/** - Rendering abstraction (Vulkan 1.3 dynamic rendering)
- `Subpass<SlotEnum>`: Base class with lazy image allocation (cached by slot/width/height/frame)
- `ScreenSpacePass<SlotEnum>`: Full-screen post-processing helpers
  - `create_default_sampler()`: Linear filtering, clamp-to-edge
  - `render_fullscreen()`: Handles RenderingInfo, viewport, binding, push constants
- `create_screen_space_pipeline()`: Pipeline factory for screen-space passes
- `SkyParameters`: Physical sky parameter system
  - `SkyParametersGPU`: 96-byte GPU-compatible struct (matches GLSL)
  - `SkyParameters`: CPU-side physical parameters
  - `create_earth_sun()`: Preconfigured Earth-Sun system
  - Uses radiance values (cd/m²), not illuminance
- **Concrete Passes:**
  - `SkyPass`: Atmospheric rendering using Rayleigh/Mie scattering
  - `SunLightPass`: Direct sun lighting with ray-traced shadows
  - `SkyLightPass`: Ray-traced indirect sky lighting with progressive accumulation
  - `ToneMappingPass`: HDR→LDR conversion (ACES, Reinhard, Uncharted2, Neutral)

**Model/** - 3D assets (namespace: `vw::Model`)
- `Mesh`, `MeshManager`: Mesh data loading (via Assimp)
- `Mesh::geometry_hash()`: Hash-based geometry identity for GPU data sharing
- `Scene`: Container for mesh instances with transforms
- `MeshInstance`: Mesh + transformation pair
- **Material System** (namespace: `vw::Model::Material`):
  - `Material`: Simple struct with `MaterialTypeTag` and `material_index`
  - `BindlessMaterialManager`: Manages material handlers and texture allocations
  - Handler system: `ColoredMaterialHandler`, `TexturedMaterialHandler`, `IMaterialTypeHandler`
  - Extensible via `register_handler()` template method

**RayTracing/** - Hardware RT (namespace: `vw::rt`)
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

### Shader Infrastructure

**Common Shaders** (`VulkanWrapper/Shaders/`):
- `fullscreen.vert`: Fullscreen triangle vertex shader

**GLSL Include Files** (`VulkanWrapper/Shaders/include/`):
- `atmosphere_params.glsl`: SkyParameters struct definition
- `atmosphere_scattering.glsl`: Rayleigh/Mie scattering functions
- `sky_radiance.glsl`: Shared sky radiance computation
- `random.glsl`: Random sampling utilities

**Ray Tracing Shaders**:
- `sky_light.rgen`, `sky_light.rmiss`, `sky_light.rchit`: Indirect sky lighting

### Deferred Rendering Pipeline

The Advanced example implements a complete deferred pipeline:
1. **Z-Pass**: Depth prepass
2. **Color Pass**: G-Buffer (position, normal, albedo, tangent, bitangent)
3. **AO Pass**: Ambient occlusion
4. **Sky Pass**: Atmospheric background
5. **Sun Light Pass**: Direct sun with ray-traced shadows
6. **Sky Light Pass**: Indirect sky lighting (progressive accumulation)
7. **Tone Mapping**: HDR→LDR with indirect blending

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

**Test Suites:**
- `MemoryTests`, `ImageTests`, `VulkanTests`: Core functionality
- `RayTracingTests`: BLAS/TLAS and RT pipeline
- `UtilsTests`, `ShaderTests`: Utilities and shader compilation
- `RenderPassTests`: All render pass implementations
- `Random/RandomSamplingTests.cpp`: Hemisphere sampling validation
- `Material/`: Comprehensive material system tests

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

- **Namespaces**: `vw` (main), `vw::Barrier`, `vw::Model`, `vw::Model::Material`, `vw::rt` (ray tracing)
- **Headers**: `.h` extension, `#pragma once`
- **Naming**: `snake_case` for functions/variables, `PascalCase` for types
- **Strong types**: `Width`, `Height`, `Depth`, `MipLevel` for type-safe dimensions
- **Vulkan-Hpp**: Always use `vk::` types, not `Vk` C types
- **Dynamic dispatcher**: Configured via `3rd_party.h` (no global dispatcher)
- **GLM config**: `GLM_FORCE_RADIANS`, `GLM_FORCE_DEPTH_ZERO_TO_ONE`
- **Push constants**: Passes use push constants for parameters (< 128 bytes)

### Code Formatting (clang-format)

The project uses clang-format for consistent code formatting. Configuration is in `.clang-format` at the repository root.

**Key formatting settings:**
- Column limit: 80 characters
- Indentation: 4 spaces (no tabs)
- Pointer/reference alignment: Right (`int *ptr`, `int &ref`)
- Brace style: Attach (K&R style)
- Constructor initializers: Break before comma

**Format all source files:**
```bash
# Using find + xargs (recommended)
find . -type f \( -name "*.h" -o -name "*.cpp" \) ! -path "*/build*" ! -path "*/vcpkg*" -print0 | xargs -0 clang-format -i

# Or format a single file
clang-format -i path/to/file.cpp
```

**Check formatting without modifying:**
```bash
clang-format --dry-run -Werror path/to/file.cpp
```

**IDE integration:**
- Most IDEs (CLion, VS Code, etc.) detect `.clang-format` automatically
- Enable "Format on Save" for seamless formatting

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

### Agent-Assisted Development Workflow

For complex graphics engine features, use specialized agents in the following order:

1. **Architecture & Planning** (`rendering-engine-architect`)
   - Design implementation approach and rendering pipeline changes
   - Evaluate performance trade-offs and architectural decisions
   - Plan shader systems and resource management strategies
   - Create frame graph designs and rendering pass structures

2. **Write Tests** (`graphics-engine-tester`)
   - Write unit tests for the planned feature (TDD approach)
   - Create tests for GPU resources, memory management, and shader compilation
   - Add tests for ray tracing features and rendering pipelines
   - Verify tests compile and fail appropriately before implementation

3. **Implement Feature** (`graphics-engine-dev`)
   - Implement the feature following the architectural plan
   - Write Vulkan-based rendering code, shader programs, and GPU optimizations
   - Handle synchronization, memory barriers, and resource management
   - Iterate until all tests pass

4. **Code Review** (`graphics-engine-reviewer`)
   - Review rendering pipeline correctness and performance
   - Verify synchronization logic and memory management patterns
   - Check shader code quality and ray tracing implementations
   - Ensure adherence to project patterns and best practices

**Example workflow for adding a new render pass:**
```
User: Add a screen-space reflections pass

1. rendering-engine-architect → Plans SSR algorithm, buffer requirements, integration points
2. graphics-engine-tester    → Writes tests for SSRPass class and shader compilation
3. graphics-engine-dev       → Implements SSRPass, shaders, and pipeline integration
4. graphics-engine-reviewer  → Reviews implementation for correctness and performance
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
- `VulkanWrapper/include/VulkanWrapper/RenderPass/SkyParameters.h`: Physical sky system
- `VulkanWrapper/include/VulkanWrapper/Random/RandomSamplingBuffer.h`: Random sampling infrastructure
