# VulkanWrapper Library

Core Vulkan wrapper library organized into 15 modules. Headers at `include/VulkanWrapper/<Module>/`, sources at `src/VulkanWrapper/<Module>/`.

## Module Map

| Module | Namespace | Key Types | Tier |
|--------|-----------|-----------|------|
| [Utils](Utils/README.md) | `vw` | Exception hierarchy, ObjectWithHandle, check_vk/check_sdl/check_vma | 1 |
| [Shader](Shader/README.md) | `vw` | ShaderCompiler (GLSL→SPIR-V via shaderc) | 1 |
| [Window](Window/README.md) | `vw` | Window, SDL_Initializer, WindowBuilder | 1 |
| [Vulkan](Vulkan/README.md) | `vw` | Instance, Device, DeviceFinder, Queue, PresentQueue, Swapchain, PhysicalDevice, Surface | 2 |
| [Image](Image/README.md) | `vw` | Image, ImageView, Sampler, CombinedImage, Mipmap, ImageLoader | 2 |
| [Synchronization](Synchronization/README.md) | `vw`, `vw::Barrier` | ResourceTracker, Fence, Semaphore, Interval, IntervalSet | 2-3 |
| [Command](Command/README.md) | `vw` | CommandPool, CommandBuffer, CommandBufferRecorder | 3 |
| [Memory](Memory/README.md) | `vw` | Allocator, Buffer\<T,HostVisible,Flags\>, Transfer, StagingBufferManager, UniformBufferAllocator | 4 |
| [Descriptors](Descriptors/README.md) | `vw` | DescriptorSet, DescriptorSetLayout, DescriptorPool, DescriptorAllocator, Vertex concept | 4 |
| [Pipeline](Pipeline/README.md) | `vw` | GraphicsPipelineBuilder, Pipeline, PipelineLayout, ShaderModule, ComputePipeline, ColorBlendConfig | 4 |
| [Model](Model/README.md) | `vw::Model` | Scene, Mesh, MeshManager, Importer | 5 |
| [Model/Material](Model/Material/README.md) | `vw::Model::Material` | BindlessMaterialManager, BindlessTextureManager, IMaterialTypeHandler, Material handlers | 5 |
| [Random](Random/README.md) | `vw` | NoiseTexture, RandomSamplingBuffer | 5 |
| [RayTracing](RayTracing/README.md) | `vw::rt` | RayTracedScene, RayTracingPipeline, BLAS, TLAS, ShaderBindingTable, GeometryReference | 6 |
| [RenderPass](RenderPass/README.md) | `vw` | Subpass\<SlotEnum\>, ScreenSpacePass, SkyPass, ToneMappingPass, IndirectLightPass | 6 |

## Adding New Code

1. **Header**: `include/VulkanWrapper/<Category>/NewClass.h`
2. **Source**: `src/VulkanWrapper/<Category>/NewClass.cpp`
3. **CMake**: Add to both `include/VulkanWrapper/<Category>/CMakeLists.txt` (PUBLIC) and `src/VulkanWrapper/<Category>/CMakeLists.txt` (PRIVATE)
4. **PCH**: `3rd_party.h` is pre-compiled — no manual include needed
5. **Tests**: `tests/<Category>/NewClassTests.cpp`, register in `tests/CMakeLists.txt`

```cmake
# Header CMakeLists.txt
target_sources(VulkanWrapperCoreLibrary PUBLIC NewClass.h)
# Source CMakeLists.txt
target_sources(VulkanWrapperCoreLibrary PRIVATE NewClass.cpp)
```
