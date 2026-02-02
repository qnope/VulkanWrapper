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
```

**Note:** Uses llvmpipe (software Vulkan driver with full ray tracing support).

## Directory Structure

```
VulkanWrapper/
├── include/VulkanWrapper/   # Public headers
│   ├── Memory/              # Allocator, Buffer<T,HostVisible,Usage>, ResourceTracker
│   ├── RenderPass/          # ScreenSpacePass, SkyPass, ToneMappingPass
│   ├── RayTracing/          # BLAS/TLAS, RayTracedScene
│   └── ...
├── tests/                   # GTest tests (use create_gpu() singleton)
└── Shaders/include/         # GLSL includes (random.glsl, atmosphere_*.glsl)
```

## Core Patterns

**Type-Safe Buffers:**
```cpp
Buffer<T, HostVisible, Usage>  // e.g., Buffer<float, true, UniformBufferUsage>
```

**ResourceTracker for Barriers:**
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

- **Namespaces:** `vw`, `vw::rt`, `vw::Model`, `vw::Barrier`
- **Naming:** `snake_case` functions, `PascalCase` types, `m_member` members
- **Types:** Use `vk::` not `Vk`, strong types (`Width`, `Height`, `MipLevel`)
- **Format:** 80 columns, 4 spaces, clang-format enforced

## Error Handling

```cpp
check_vk(result, "context");   // VulkanException
check_vma(result, "context");  // VMAException
check_sdl(ptr, "context");     // SDLException
```

## Test GPU Singleton

```cpp
auto& gpu = vw::tests::create_gpu();
// gpu.device, gpu.allocator, gpu.queue()
```
