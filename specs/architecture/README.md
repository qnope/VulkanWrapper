# VulkanWrapper Architecture

C++23 Vulkan 1.3 wrapper library providing RAII resource management, type safety via templates and strong types, builder patterns for all complex objects, and hardware ray tracing support. Uses dynamic rendering (no `VkRenderPass`), Synchronization2, and bindless resources.

## Layer Architecture

```
Tier 6:  RayTracing                    RenderPass
         (BLAS, TLAS, RayTracedScene,  (Subpass, ScreenSpacePass, SkyPass,
          RayTracingPipeline, SBT)      ToneMappingPass, IndirectLightPass)
              |                              |
Tier 5:  Model                         Random
         (Scene, Mesh, MeshManager,    (NoiseTexture,
          Material subsystem)           RandomSamplingBuffer)
              |                              |
Tier 4:  Memory                   Descriptors              Pipeline
         (Allocator, Buffer,      (DescriptorSet,           (Pipeline, PipelineLayout,
          Transfer,                DescriptorSetLayout,       ShaderModule,
          StagingBufferManager)    DescriptorPool, Vertex)    ComputePipeline)
              |
Tier 3:  Command
         (CommandPool, CommandBuffer)
              |
Tier 2:  Vulkan                                Synchronization
         (Image, ImageView, Sampler,           (Interval, IntervalSet,
          Instance, Device, DeviceFinder,       ResourceTracker)
          Queue, Swapchain, Fence, Semaphore)
              |
Tier 1:  Utils              Shader              Window
         (Error, Algos,     (ShaderCompiler)     (SDL3 Window,
          Alignment)                              SDL_Initializer)
              |
Tier 0:  std3rd  vulkan3rd  vma3rd  glm3rd  assimp3rd  sdl3rd  shaderc3rd
```

Each tier depends only on tiers below it. Tier 1 modules have no inter-dependencies.

## Core Design Patterns

**Builder pattern** — all complex Vulkan objects:
```cpp
auto instance  = InstanceBuilder().setDebug().setApiVersion(ApiVersion::e13).build();
auto device    = instance->findGpu().with_queue(eGraphics).with_dynamic_rendering().build();
auto allocator = AllocatorBuilder(instance, device).build();
```

**RAII via `ObjectWithHandle<T>`** — ownership encoded in the type:
```cpp
ObjectWithHandle<vk::Pipeline>        // non-owning
ObjectWithHandle<vk::UniquePipeline>  // owning, auto-destroyed
```

**Type-safe Buffers** — usage and visibility as template parameters:
```cpp
auto buf = create_buffer<float, true, UniformBufferUsage>(*allocator, 100);
```

**ResourceTracker** — automatic barrier management (namespace `vw::Barrier`):
```cpp
tracker.track(ImageState{image, range, eUndefined, eTopOfPipe, eNone});
tracker.request(ImageState{image, range, eColorAttachmentOptimal, eColorAttachmentOutput, eColorAttachmentWrite});
tracker.flush(cmd);
```

**Subpass / ScreenSpacePass hierarchy** — lazy image allocation, fullscreen quad rendering:
```cpp
Subpass<SlotEnum>          // base: get_or_create_image() returns CachedImage
ScreenSpacePass<SlotEnum>  // derived: render_fullscreen() for post-processing
```

## Namespaces

| Namespace | Scope |
|-----------|-------|
| `vw` | Core library types |
| `vw::rt` | Ray tracing (BLAS, TLAS, pipelines) |
| `vw::Model` | Scene, Mesh, Importer |
| `vw::Model::Material` | Bindless material system, handlers |
| `vw::Barrier` | ResourceTracker, ResourceState types |
| `vw::tests` | Test utilities, GPU singleton |

## Subdirectories

- [VulkanWrapper Library](VulkanWrapper/README.md) — core library modules
- [Shaders](Shaders/README.md) — GLSL shaders and includes
- [Tests](tests/README.md) — GTest test suites
- [Examples](examples/README.md) — sample applications (basic + advanced deferred/RT)
