# CLAUDE.md

Modern C++23 Vulkan 1.3 wrapper library with RAII, type safety, and ray tracing support.

## Build & Test

```bash
# Build
cmake -B build-Clang20Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-Clang20Debug

# Run all tests
cd build-Clang20Debug && ctest

# Run specific test
cd build-Clang20Debug/VulkanWrapper/tests && ./RenderPassTests

# Format code
clang-format -i <file>
```

**Note:** Uses llvmpipe (software Vulkan driver with full ray tracing support).

**Dependencies (vcpkg):** sdl3, sdl3-image, glm, assimp, vulkan, shaderc, vulkan-memory-allocator, gtest

## Directory Structure

```
VulkanWrapper/
├── include/VulkanWrapper/   # Public headers
│   ├── Command/             # CommandBuffer, CommandPool
│   ├── Descriptors/         # DescriptorSet, DescriptorSetLayout, Vertex
│   ├── Image/               # Image, ImageView, Sampler, CombinedImage, Mipmap
│   ├── Memory/              # Allocator, Buffer<T,HostVisible,Usage>, Barrier helpers
│   ├── Model/               # Scene, Mesh, MeshManager, Importer
│   │   └── Material/        # BindlessMaterialManager, MaterialTypeHandler
│   ├── Pipeline/            # Pipeline (GraphicsPipelineBuilder), PipelineLayout, ShaderModule
│   ├── Random/              # NoiseTexture, RandomSamplingBuffer
│   ├── RayTracing/          # BLAS/TLAS, RayTracedScene, RayTracingPipeline, ShaderBindingTable
│   ├── RenderPass/          # Subpass (lazy image cache), ScreenSpacePass, SkyPass, ToneMappingPass
│   ├── Shader/              # ShaderCompiler (GLSL → SPIR-V)
│   ├── Synchronization/     # ResourceTracker, Fence, Semaphore
│   ├── Utils/               # Error (exception hierarchy), ObjectWithHandle, Algos
│   ├── Vulkan/              # Instance, Device, DeviceFinder, Queue, Swapchain
│   └── Window/              # SDL3 Window
├── tests/                   # GTest tests (use create_gpu() singleton)
├── Shaders/include/         # GLSL includes (random.glsl, atmosphere_*.glsl)
examples/                    # Advanced (deferred rendering), Application (basic)
```

## Core Patterns

**Type-Safe Buffers:**
```cpp
Buffer<T, HostVisible, Usage>  // e.g., Buffer<float, true, UniformBufferUsage>
```

**Subpass → ScreenSpacePass hierarchy:**
```cpp
Subpass<SlotEnum>            // Base: lazy image allocation via get_or_create_image()
ScreenSpacePass<SlotEnum>    // Derived: fullscreen quad rendering (4 vertices, triangle strip)
```

**ResourceTracker for Barriers** (in `vw::Barrier`):
```cpp
tracker.track(ImageState{...});    // Current state
tracker.request(ImageState{...});  // Needed state
tracker.flush(cmd);                // Generate barriers
```

**Dynamic Rendering (no VkRenderPass):**
```cpp
cmd.beginRendering(vk::RenderingInfo{...});
// draw
cmd.endRendering();
```

## Code Style

- **Namespaces:** `vw`, `vw::rt`, `vw::Model`, `vw::Model::Material`, `vw::Barrier`, `vw::tests`
- **Naming:** `snake_case` functions, `PascalCase` types, `m_member` members
- **Types:** Use `vk::` not `Vk`, strong types (`Width`, `Height`, `MipLevel`)
- **Format:** 80 columns, 4 spaces, `.clang-format` at root (LLVM-based)

## Error Handling

```cpp
check_vk(result, "context");        // VulkanException
check_vma(result, "context");       // VMAException
check_sdl(ptr_or_bool, "context");  // SDLException (pointer and bool overloads)
```

Exception hierarchy: `vw::Exception` → `VulkanException`, `VMAException`, `SDLException`, `FileException`, `AssimpException`, `ShaderCompilationException`, `ValidationLayerException`, `SwapchainException`, `LogicException`

## Test GPU Singleton

```cpp
auto& gpu = vw::tests::create_gpu();
// gpu.device, gpu.allocator, gpu.queue()

// For ray tracing tests:
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```
