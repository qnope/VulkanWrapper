# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VulkanWrapper is a C++23 library that provides a modern, high-level abstraction over the Vulkan graphics API. The library wraps Vulkan's verbose C-style API with RAII-based objects, automatic resource management, and simplified interfaces for common operations.

**Key Features:**
- Modern C++23 interface with concepts and ranges
- RAII-based resource management using Vulkan-Hpp unique handles
- Automatic memory management via Vulkan Memory Allocator (VMA)
- Ray tracing pipeline support (BLAS/TLAS, shader binding tables)
- Resource state tracking and automatic barrier insertion
- Shader compilation integration with glslangValidator
- Cross-language bindings (C API in `vw_c/`, Python in `vw_py/`, Rust in `vw_rust/`)

## Build System

**Build tool:** CMake 3.25+ with vcpkg for dependencies

**Build configurations:**
```bash
# Configure with vcpkg toolchain (automatic via CMakePresets or manual)
cmake -B build-Clang20Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake -B build-Clang20Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build-Clang20Debug
cmake --build build-Clang20Release
```

**Dependencies (via vcpkg.json):**
- SDL3 with Vulkan support (windowing)
- SDL3-image (texture loading)
- GLM (mathematics)
- Assimp (3D model loading)
- Vulkan Memory Allocator (memory management)
- GTest (testing)

**Shader compilation:** Use the `VwCompileShader()` CMake function to compile GLSL shaders:
```cmake
VwCompileShader(TargetName path/to/shader.vert output/path/shader.spv)
VwCompileShader(TargetName path/to/shader.rgen output.rgen.spv --target-env vulkan1.2)
```

## Testing

**Run all tests:**
```bash
cd build-Clang20Debug
ctest
```

**Run specific test suite:**
```bash
cd build-Clang20Debug
./VulkanWrapper/tests/MemoryTests
```

**Run specific test:**
```bash
cd build-Clang20Debug
./VulkanWrapper/tests/MemoryTests --gtest_filter=ResourceTrackerTests.TrackAndRequestImageBarrier
```

Tests are located in `VulkanWrapper/tests/` with utilities in `VulkanWrapper/tests/utils/`.

## Architecture

### Core Namespace Organization

**`vw::`** - Root namespace containing all library components

**`vw::Barrier::`** - Resource state tracking and synchronization
- `ResourceTracker`: Automatic barrier generation based on tracked/requested resource states
- `ImageState`, `BufferState`, `AccelerationStructureState`: Resource state descriptors
- Provides automatic pipeline barrier insertion to prevent data hazards

**`vw::Model::`** - 3D model and material management
- `Mesh`: Geometry data with vertex/index buffers
- `Model::Material::Material`: Material properties and textures
- `Model::Material::MaterialManager`: Material instance management
- `Model::Internal::MaterialInfo`: Internal material representation

### Key Architectural Patterns

**Device-centric design:** Most objects require a `Device` reference at construction. The `Device` class wraps a logical Vulkan device and maintains queues (graphics, compute, transfer, present).

**Handle ownership:** Uses `vulkan.hpp` unique handles (`vk::UniqueDevice`, `vk::UniqueCommandBuffer`, etc.) wrapped in custom classes inheriting from `ObjectWithUniqueHandle<T>` or `ObjectWithHandle<T>`.

**Memory management layers:**
1. `Allocator` - VMA wrapper for allocation/deallocation
2. `Buffer` and `BufferBase` - Typed and untyped buffer abstractions
3. `StagingBufferManager` - Manages temporary staging buffers for transfers
4. `Transfer` namespace - High-level transfer operations
5. `UniformBufferAllocator` - Sub-allocation from large uniform buffers
6. `IntervalSet` and `Interval` - Track allocated regions within buffers

**Resource tracking:** The `ResourceTracker` class (in `Barrier` namespace) tracks the last known state of resources (images, buffers, acceleration structures) and automatically generates pipeline barriers when resources transition states. Use `track()` to record current state and `request()` to specify needed state, then `flush()` to insert barriers.

**Pipeline abstractions:**
- `Pipeline`: Graphics/compute pipelines
- `PipelineLayout`: Descriptor set layouts and push constants
- `RayTracingPipeline`: Ray tracing pipeline with shader groups
- `ShaderModule`: SPIR-V shader module wrapper
- `MeshRenderer`: High-level mesh rendering interface

**Ray tracing:** Full support via:
- `BottomLevelAccelerationStructure` (BLAS): Per-geometry acceleration structures
- `TopLevelAccelerationStructure` (TLAS): Scene-level instance acceleration structure
- `ShaderBindingTable`: Ray tracing shader binding table management
- `RayTracingPipeline`: Pipeline with ray generation, miss, hit, and callable shaders

**Render pass management:**
- `Rendering` and `RenderingBuilder`: Dynamic rendering (Vulkan 1.3+)
- `Subpass`: Render pass subpass abstraction
- `ScreenSpacePass`: Full-screen effects helper

**Windowing and presentation:**
- `Window`: SDL3 window wrapper
- `Surface`: Vulkan surface for presentation
- `Swapchain` and `SwapchainBuilder`: Swapchain creation and management
- `PresentQueue`: Queue specialized for presentation

### Critical Files

**Core device setup:**
- `VulkanWrapper/include/VulkanWrapper/Vulkan/Instance.h` - Vulkan instance creation
- `VulkanWrapper/include/VulkanWrapper/Vulkan/PhysicalDevice.h` - Physical device enumeration
- `VulkanWrapper/include/VulkanWrapper/Vulkan/DeviceFinder.h` - Device selection and creation
- `VulkanWrapper/include/VulkanWrapper/Vulkan/Device.h` - Logical device wrapper

**Memory system:**
- `VulkanWrapper/include/VulkanWrapper/Memory/Allocator.h` - VMA wrapper
- `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h` - Buffer abstraction
- `VulkanWrapper/include/VulkanWrapper/Memory/StagingBufferManager.h` - Staging buffer pool
- `VulkanWrapper/include/VulkanWrapper/Memory/UniformBufferAllocator.h` - Uniform buffer sub-allocator
- `VulkanWrapper/include/VulkanWrapper/Memory/Transfer.h` - Transfer operations

**Synchronization:**
- `VulkanWrapper/include/VulkanWrapper/Synchronization/ResourceTracker.h` - Automatic barrier generation
- `VulkanWrapper/include/VulkanWrapper/Synchronization/Fence.h` - CPU-GPU synchronization
- `VulkanWrapper/include/VulkanWrapper/Synchronization/Semaphore.h` - GPU-GPU synchronization
- `VulkanWrapper/include/VulkanWrapper/Memory/Barrier.h` - Manual barrier helpers

**Precompiled header:**
- `VulkanWrapper/include/VulkanWrapper/3rd_party.h` - Common includes (vulkan.hpp, GLM, STL)

### Forward Declarations

`VulkanWrapper/include/VulkanWrapper/fwd.h` provides forward declarations for all major classes to minimize header dependencies. Include this instead of full headers when possible.

## Development Notes

**C++ Standard:** Requires C++23 (see `target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)`)

**Vulkan-Hpp configuration:**
- Uses dynamic dispatcher: `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1`
- No exceptions: `VULKAN_HPP_NO_EXCEPTIONS 1`
- Asserts on result errors: `VULKAN_HPP_ASSERT_ON_RESULT`

**Naming conventions:**
- Classes: PascalCase
- Methods/functions: snake_case
- Member variables: `m_camelCase` prefix

**Examples:**
- `examples/Application/` - Basic application framework (reusable `App` library)
- `examples/Advanced/` - Full rendering example with G-buffer, ray tracing, post-processing

**Library variants:**
- Main C++ library: `VulkanWrapperCoreLibrary` (alias `VulkanWrapper::VW`)
- C bindings: `vw_c/` (currently disabled in root CMakeLists.txt)
- Python bindings: `vw_py/` (currently disabled in root CMakeLists.txt)
- Rust bindings: `vw_rust/` (manual build process)

**Clang-format:** Project uses `.clang-format` for consistent code style.
