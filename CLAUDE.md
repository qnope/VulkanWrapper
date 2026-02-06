# CLAUDE.md

Modern C++23 Vulkan 1.3 wrapper library with RAII, type safety, and ray tracing support.

## Build & Test

```bash
# Configure (first time only — vcpkg is auto-detected via VCPKG_ROOT or CMAKE_TOOLCHAIN_FILE)
cmake -B build-Clang20Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build-Clang20Debug

# Build specific test target
cmake --build build-Clang20Debug --target RenderPassTests

# Run all tests
cd build-Clang20Debug && ctest --output-on-failure

# Run specific test
cd build-Clang20Debug/VulkanWrapper/tests && ./RenderPassTests

# Filter tests
cd build-Clang20Debug/VulkanWrapper/tests && ./RenderPassTests --gtest_filter='*IndirectLight*'

# Format changed files (staged only)
git diff --cached --name-only -- '*.h' '*.cpp' | xargs -r clang-format -i

# Format all changed files (staged + unstaged)
git diff --name-only -- '*.h' '*.cpp' | xargs -r clang-format -i
```

**Note:** Uses llvmpipe (software Vulkan driver with full ray tracing support).

**Dependencies (vcpkg):** sdl3, sdl3-image, glm, assimp, vulkan, shaderc, vulkan-memory-allocator, gtest

## Directory Structure

```
VulkanWrapper/                          # Library root (paths below are relative to repo root)
├── include/VulkanWrapper/              # Public headers
│   ├── 3rd_party.h                     # Third-party includes (used as PCH)
│   ├── fwd.h                           # Forward declarations
│   ├── Command/                        # CommandBuffer, CommandPool
│   ├── Descriptors/                    # DescriptorSet, DescriptorSetLayout, DescriptorPool, DescriptorAllocator, Vertex
│   ├── Image/                          # Image, ImageView, Sampler, CombinedImage, Mipmap, ImageLoader
│   ├── Memory/                         # Allocator, Buffer, BufferBase, BufferList, BufferUsage, AllocateBufferUtils,
│   │                                   #   Transfer, StagingBufferManager, Barrier, UniformBufferAllocator, Interval, IntervalSet
│   ├── Model/                          # Scene, Mesh, MeshManager, Importer
│   │   ├── Internal/                   # MaterialInfo, MeshInfo (internal implementation details)
│   │   └── Material/                   # IMaterialTypeHandler, BindlessMaterialManager, BindlessTextureManager,
│   │                                   #   TexturedMaterialHandler, ColoredMaterialHandler, MaterialTypeTag, MaterialPriority
│   ├── Pipeline/                       # GraphicsPipelineBuilder, Pipeline, PipelineLayout, ShaderModule, MeshRenderer, ColorBlendConfig
│   ├── Random/                         # NoiseTexture, RandomSamplingBuffer
│   ├── RayTracing/                     # BLAS/TLAS, RayTracedScene, RayTracingPipeline, ShaderBindingTable, GeometryReference
│   ├── RenderPass/                     # Subpass, ScreenSpacePass, SkyPass, ToneMappingPass, SunLightPass, IndirectLightPass
│   ├── Shader/                         # ShaderCompiler (GLSL -> SPIR-V via shaderc)
│   ├── Synchronization/               # ResourceTracker (vw::Barrier namespace), Fence, Semaphore
│   ├── Utils/                          # Error (exception hierarchy), ObjectWithHandle, Algos, Alignment
│   ├── Vulkan/                         # Instance, Device, DeviceFinder, Queue, PresentQueue, Swapchain, PhysicalDevice, Surface
│   └── Window/                         # SDL3 Window, SDL_Initializer
├── src/VulkanWrapper/                  # Implementation (.cpp files, mirrors include/ structure)
├── tests/                              # GTest tests (use create_gpu() singleton)
│   └── utils/create_gpu.hpp           # GPU singleton for tests
├── Shaders/
│   ├── fullscreen.vert                 # Fullscreen quad from vertex ID (4 verts, triangle strip)
│   └── include/                        # GLSL includes (random.glsl, atmosphere_*.glsl, geometry_access.glsl, sky_radiance.glsl)
examples/                               # Advanced (deferred rendering + RT), Application (basic)
```

## Adding New Code

When creating a new class in an existing module:
1. **Header**: `VulkanWrapper/include/VulkanWrapper/<Category>/NewClass.h`
2. **Source**: `VulkanWrapper/src/VulkanWrapper/<Category>/NewClass.cpp`
3. **Register in CMake**: Add filename to both `include/VulkanWrapper/<Category>/CMakeLists.txt` (PUBLIC) and `src/VulkanWrapper/<Category>/CMakeLists.txt` (PRIVATE)
4. **PCH**: `3rd_party.h` is pre-compiled — no need to include it manually
5. **Tests**: Add test file to `VulkanWrapper/tests/<Category>/NewClassTests.cpp`, register in `VulkanWrapper/tests/CMakeLists.txt`

Example CMake registration (header):
```cmake
target_sources(VulkanWrapperCoreLibrary PUBLIC NewClass.h)
```
Example CMake registration (source):
```cmake
target_sources(VulkanWrapperCoreLibrary PRIVATE NewClass.cpp)
```

## Core Patterns

**Builder Pattern** (all complex objects):
```cpp
auto instance = InstanceBuilder().setDebug().setApiVersion(ApiVersion::e13).build();
auto device = instance->findGpu().with_queue(eGraphics).with_synchronization_2().with_dynamic_rendering().build();
auto allocator = AllocatorBuilder(instance, device).build();
// Also: ImageViewBuilder, SamplerBuilder, CommandPoolBuilder, FenceBuilder,
//       GraphicsPipelineBuilder, RayTracingPipelineBuilder, DescriptorSetLayoutBuilder, ...
```

**Type-Safe Buffers** (`Memory/Buffer.h`, `Memory/AllocateBufferUtils.h`):
```cpp
// Template: Buffer<typename T, bool HostVisible, VkBufferUsageFlags Flags>
// Create via: create_buffer<T, HostVisible, UsageConstant>(allocator, count)
// Usage constants (BufferUsage.h): VertexBufferUsage, IndexBufferUsage, UniformBufferUsage, StorageBufferUsage, StagingBufferUsage
auto buf = create_buffer<float, true, UniformBufferUsage>(*allocator, 100);
// Also: allocate_vertex_buffer<VertexType, HostVisible>(allocator, count)
```

**RAII Handles** (`ObjectWithHandle<T>`, ownership via template parameter):
```cpp
ObjectWithHandle<vk::Pipeline>          // Non-owning
ObjectWithHandle<vk::UniquePipeline>    // Owning (auto-destroyed)
// ObjectWithUniqueHandle<T> is a type alias, not a separate class
// to_handle range adaptor: objects | vw::to_handle | to<std::vector>
```

**Subpass -> ScreenSpacePass hierarchy** (`RenderPass/`):
```cpp
Subpass<SlotEnum>            // Base: lazy image allocation via get_or_create_image(), returns CachedImage{image, view}
ScreenSpacePass<SlotEnum>    // Derived: fullscreen quad rendering via render_fullscreen()
// Helper: create_screen_space_pipeline(device, vert, frag, layout, format, ...) in ScreenSpacePass.h
```

**ResourceTracker for Barriers** (namespace `vw::Barrier`, `Synchronization/ResourceTracker.h`):
```cpp
vw::Barrier::ResourceTracker tracker;
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.flush(cmd);
// Supports: ImageState, BufferState, AccelerationStructureState
```

**Transfer** (`Memory/Transfer.h`) wraps copy ops with automatic barrier management:
```cpp
vw::Transfer transfer;
transfer.blit(cmd, srcImage, dstImage);
transfer.copyBufferToImage(cmd, srcBuffer, dstImage, srcOffset);
transfer.saveToFile(cmd, allocator, queue, image, "output.png");
auto& tracker = transfer.resourceTracker();  // Access embedded ResourceTracker
```

**Dynamic Rendering (no VkRenderPass):**
```cpp
cmd.beginRendering(vk::RenderingInfo{...});
// draw
cmd.endRendering();
```

**MeshRenderer** (`Pipeline/MeshRenderer.h`): binds pipelines per `MaterialTypeTag` for bindless mesh rendering.

## Code Style

- **Namespaces:** `vw`, `vw::rt`, `vw::Model`, `vw::Model::Material`, `vw::Barrier`, `vw::tests`
- **Naming:** `snake_case` functions, `PascalCase` types, `m_member` members
- **Types:** Use `vk::` not `Vk`, strong types (`Width`, `Height`, `MipLevel`)
- **Format:** 80 columns, 4 spaces, `.clang-format` at root (LLVM-based)

## Anti-Patterns (DO NOT)

- **DO NOT** use `vk::PipelineStageFlagBits` (v1) -- use `vk::PipelineStageFlagBits2` (Synchronization2)
- **DO NOT** use `vk::AccessFlagBits` (v1) -- use `vk::AccessFlags2`
- **DO NOT** use `cmd.beginRenderPass()` / `VkRenderPass` / `VkFramebuffer` -- use `cmd.beginRendering()` (dynamic rendering)
- **DO NOT** use raw `vkCmdPipelineBarrier` -- use `ResourceTracker` (namespace `vw::Barrier`)
- **DO NOT** use raw `vkAllocateMemory` / `vkCreateBuffer` -- use `Allocator` and `create_buffer<>()`
- **DO NOT** use `std::enable_if_t` / SFINAE -- use C++23 concepts and `requires` clauses
- **DO NOT** write raw loops when ranges/algorithms work -- use `std::ranges::`, `std::views::`, `std::erase_if`
- **DO NOT** use `Vk` prefix types -- use `vk::` C++ bindings from vulkan.hpp

## Error Handling

```cpp
check_vk(result, "context");        // VulkanException
check_vma(result, "context");       // VMAException
check_sdl(ptr_or_bool, "context");  // SDLException (pointer and bool overloads)
```

Exception hierarchy: `vw::Exception` -> `VulkanException`, `VMAException`, `SDLException`, `FileException`, `AssimpException`, `ShaderCompilationException`, `ValidationLayerException`, `SwapchainException`, `LogicException`

## Test GPU Singleton

```cpp
auto& gpu = vw::tests::create_gpu();  // Shared singleton (tests/utils/create_gpu.hpp)
// gpu.instance, gpu.device, gpu.allocator, gpu.queue()

// For ray tracing tests (get_ray_tracing_gpu() is defined per-test-file, not in a shared header):
auto* gpu = get_ray_tracing_gpu();
if (!gpu) GTEST_SKIP() << "Ray tracing unavailable";
```

## Key API Quick Reference

| What | How | Header |
|------|-----|--------|
| Create buffer | `create_buffer<T, HostVisible, Usage>(allocator, count)` | `Memory/AllocateBufferUtils.h` |
| Create vertex buffer | `allocate_vertex_buffer<Vertex, false>(allocator, count)` | `Memory/AllocateBufferUtils.h` |
| Create image | `allocator->create_image_2D(Width{w}, Height{h}, false, fmt, usage)` | `Memory/Allocator.h` |
| Build image view | `ImageViewBuilder(device, image).set_aspect(eColor).build()` | `Image/ImageView.h` |
| Build sampler | `SamplerBuilder(device).set_filter(eLinear).build()` | `Image/Sampler.h` |
| Compile shader | `ShaderCompiler().compile_file_to_module(device, "file.frag")` | `Shader/ShaderCompiler.h` |
| Fullscreen pipeline | `create_screen_space_pipeline(device, vert, frag, layout, fmt)` | `RenderPass/ScreenSpacePass.h` |
| Batch uploads | `StagingBufferManager::fill_buffer(data, dst, offset)` | `Memory/StagingBufferManager.h` |
| Save image to disk | `transfer.saveToFile(cmd, allocator, queue, image, "out.png")` | `Memory/Transfer.h` |
| DeviceFinder | `instance->findGpu().with_queue(eGraphics).with_ray_tracing().build()` | `Vulkan/DeviceFinder.h` |
| Compile shader (CMake) | `VwCompileShader(TARGET "shader.frag" "output.spv")` | `VulkanWrapper/CMakeLists.txt` |
