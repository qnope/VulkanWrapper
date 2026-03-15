---
sidebar_position: 1
---

# Architecture

VulkanWrapper is organized as a layered library where each tier builds on the one below it. This design prevents circular dependencies, enforces a clear initialization order, and guarantees predictable destruction order through RAII.

## The Tier System

The library is divided into seven tiers. A module may only depend on modules in lower tiers, never on modules in the same tier or above.

```
┌──────────────────────────────────────────────────────────────┐
│  Tier 6   RayTracing  ·  RenderPass                         │
├──────────────────────────────────────────────────────────────┤
│  Tier 5   Model (Scene, Mesh, Materials)  ·  Random         │
├──────────────────────────────────────────────────────────────┤
│  Tier 4   Memory  ·  Descriptors  ·  Pipeline               │
├──────────────────────────────────────────────────────────────┤
│  Tier 3   Command (CommandPool, CommandBuffer)               │
├──────────────────────────────────────────────────────────────┤
│  Tier 2   Vulkan (Instance, Device, Queue, Image, …)        │
│           Synchronization (ResourceTracker, Interval)        │
├──────────────────────────────────────────────────────────────┤
│  Tier 1   Utils  ·  Shader  ·  Window                       │
├──────────────────────────────────────────────────────────────┤
│  Tier 0   Third-party (std, vulkan, vma, glm, assimp,       │
│           sdl, shaderc)                                      │
└──────────────────────────────────────────────────────────────┘
```

### Tier 0 -- Third-Party Foundations

Seven independent C++20 modules wrap external libraries so the rest of the codebase never uses raw `#include` for them:

| Module | Wraps |
|--------|-------|
| `std3rd` | Standard library headers (`<vector>`, `<memory>`, `<ranges>`, etc.) |
| `vulkan3rd` | Vulkan-Hpp C++ bindings (`<vulkan/vulkan.hpp>`) |
| `vma3rd` | Vulkan Memory Allocator |
| `glm3rd` | GLM math library |
| `assimp3rd` | Assimp model importer |
| `sdl3rd` | SDL3 windowing and input |
| `shaderc3rd` | Shaderc GLSL-to-SPIR-V compiler |

### Tier 1 -- Standalone Utilities

These modules have no Vulkan dependencies and can be used independently:

- **Utils** -- Exception hierarchy, helper algorithms, alignment utilities.
- **Shader** -- GLSL-to-SPIR-V compilation via shaderc.
- **Window** -- SDL3 window creation and event handling.

### Tier 2 -- Vulkan Core and Synchronization

The foundation of every Vulkan application:

- **Vulkan** -- Instance, Device, PhysicalDevice, DeviceFinder, Queue, PresentQueue, Swapchain, Surface, Image, ImageView, Sampler, CombinedImage, Mipmap, ImageLoader, Fence, Semaphore, Barrier.
- **Synchronization** -- `ResourceTracker` (automatic barrier generation), `Interval`, `IntervalSet`.

### Tier 3 -- Commands

- **Command** -- `CommandPool` and `CommandBuffer` for recording GPU work. Depends on the Vulkan tier for device and queue handles.

### Tier 4 -- Resources and Pipelines

- **Memory** -- `Allocator`, `Buffer`, `Transfer`, `StagingBufferManager`, `UniformBufferAllocator`, and buffer creation utilities.
- **Descriptors** -- `DescriptorSetLayout`, `DescriptorPool`, `DescriptorSet`, `DescriptorAllocator`, `Vertex` types.
- **Pipeline** -- `GraphicsPipelineBuilder`, `Pipeline`, `PipelineLayout`, `ShaderModule`, `ColorBlendConfig`.

### Tier 5 -- Scene and Randomness

- **Model** -- `Scene`, `Mesh`, `MeshManager`, `Importer`, `MeshRenderer`, and the bindless material system (`BindlessMaterialManager`, `BindlessTextureManager`, `IMaterialTypeHandler`).
- **Random** -- `NoiseTexture`, `RandomSamplingBuffer`.

### Tier 6 -- High-Level Rendering

- **RayTracing** -- `BottomLevelAccelerationStructure`, `TopLevelAccelerationStructure`, `RayTracingPipeline`, `ShaderBindingTable`, `RayTracedScene`, `GeometryReference`.
- **RenderPass** -- `Subpass`, `ScreenSpacePass`, `SkyPass`, `SunLightPass`, `IndirectLightPass`, `ToneMappingPass` (all using Vulkan 1.3 dynamic rendering, not legacy render passes).

## Module Dependency Graph

Each C++20 module corresponds to a directory and a `.cppm` aggregate file. The arrows below show "depends on":

```
Tier 6:  vw.raytracing ──┐
         vw.renderpass ──┤
                         ▼
Tier 5:  vw.model ───────┤
         vw.random ──────┤
                         ▼
Tier 4:  vw.memory ──────┤  vw.descriptors  vw.pipeline
                         ▼
Tier 3:  vw.command ─────┤
                         ▼
Tier 2:  vw.vulkan ──────┤  vw.sync
                         ▼
Tier 1:  vw.utils    vw.shader    vw.window
                         ▼
Tier 0:  std3rd  vulkan3rd  vma3rd  glm3rd  assimp3rd  sdl3rd  shaderc3rd
```

The root module `vw` (in `vw.cppm`) re-exports every submodule and all seven third-party modules, so consumers can write a single `import vw;` to get everything.

## Namespaces

VulkanWrapper uses a small, flat namespace structure:

| Namespace | Purpose | Example Types |
|-----------|---------|---------------|
| `vw` | Core library types | `Instance`, `Device`, `Allocator`, `Buffer`, `Pipeline` |
| `vw::Barrier` | Synchronization barrier utilities | `ResourceTracker`, `ImageState`, `BufferState` |
| `vw::Model` | 3D scene and mesh types | `Scene`, `Mesh`, `MeshManager` |
| `vw::Model::Material` | Bindless material system | `BindlessMaterialManager`, `IMaterialTypeHandler` |
| `vw::rt` | Ray tracing | `RayTracingPipeline`, `ShaderBindingTable` |
| `vw::tests` | Test utilities | `GPU`, `create_gpu()` |

## How Modules Map to Directories

The source tree mirrors the module structure:

```
VulkanWrapper/
├── include/VulkanWrapper/         # Public headers / module interface files
│   ├── Utils/                     # vw.utils
│   ├── Shader/                    # vw.shader
│   ├── Window/                    # vw.window
│   ├── Vulkan/                    # vw.vulkan (part: Instance, Device, Queue, …)
│   ├── Image/                     # vw.vulkan (part: Image, ImageView, Sampler, …)
│   ├── Synchronization/           # vw.sync (ResourceTracker, Interval, IntervalSet)
│   ├── Command/                   # vw.command
│   ├── Memory/                    # vw.memory
│   ├── Descriptors/               # vw.descriptors
│   ├── Pipeline/                  # vw.pipeline
│   ├── Model/                     # vw.model
│   ├── Random/                    # vw.random
│   ├── RayTracing/                # vw.raytracing
│   └── RenderPass/                # vw.renderpass
├── src/VulkanWrapper/             # Implementation files (mirrors include/ structure)
└── tests/                         # GTest test files
```

Each module has an aggregate `.cppm` file (e.g., `Core/Vulkan.cppm`, `Core/Memory.cppm`) that re-exports all of its partitions.

## Initialization Order

Because of the tier system, you always initialize objects top-down:

```cpp
// 1. Create a Vulkan instance (Tier 2)
auto instance = InstanceBuilder()
    .setDebug()
    .setApiVersion(ApiVersion::e13)
    .build();

// 2. Find and create a logical device (Tier 2)
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// 3. Create a memory allocator (Tier 4)
auto allocator = AllocatorBuilder(instance, device).build();

// 4. Now you can create buffers, images, pipelines, etc.
```

RAII ensures that destruction happens in the reverse order automatically -- you never need to manually tear anything down.
