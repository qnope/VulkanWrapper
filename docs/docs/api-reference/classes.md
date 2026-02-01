---
sidebar_position: 2
---

# Classes

Quick reference for main classes in VulkanWrapper.

## Vulkan Foundation

| Class | Description |
|-------|-------------|
| `Instance` | Vulkan instance wrapper |
| `InstanceBuilder` | Fluent instance creation |
| `Device` | Logical device with queues |
| `DeviceFinder` | GPU selection and device creation |
| `PhysicalDevice` | Physical GPU properties |
| `Queue` | Command queue |
| `PresentQueue` | Presentation queue |
| `Surface` | Window surface |
| `Swapchain` | Presentation swapchain |

## Memory Management

| Class | Description |
|-------|-------------|
| `Allocator` | VMA-based memory allocator |
| `AllocatorBuilder` | Allocator creation |
| `Buffer<T, HostVisible, Flags>` | Type-safe buffer |
| `BufferBase` | Base buffer class |
| `BufferList<T, Flags>` | Dynamic buffer allocation |
| `StagingBufferManager` | Batched CPU-GPU transfers |
| `UniformBufferAllocator` | Aligned uniform allocation |

## Image Handling

| Class | Description |
|-------|-------------|
| `Image` | Vulkan image wrapper |
| `ImageBuilder` | Image creation |
| `ImageView` | Image view wrapper |
| `ImageViewBuilder` | View creation |
| `Sampler` | Texture sampler |
| `SamplerBuilder` | Sampler creation |
| `CombinedImage` | ImageView + Sampler pair |
| `ImageLoader` | Load images from disk |

## Pipeline

| Class | Description |
|-------|-------------|
| `Pipeline` | Graphics/compute pipeline |
| `GraphicsPipelineBuilder` | Pipeline creation |
| `PipelineLayout` | Descriptor/push constant layout |
| `PipelineLayoutBuilder` | Layout creation |
| `ShaderModule` | SPIR-V shader module |
| `ShaderCompiler` | GLSL to SPIR-V compilation |
| `MeshRenderer` | Material-based mesh rendering |

## Descriptors

| Class | Description |
|-------|-------------|
| `DescriptorSetLayout` | Descriptor binding layout |
| `DescriptorSetLayoutBuilder` | Layout creation |
| `DescriptorPool` | Descriptor allocation pool |
| `DescriptorPoolBuilder` | Pool creation |
| `DescriptorSet` | Allocated descriptor set |
| `DescriptorAllocator` | High-level descriptor management |

## Command

| Class | Description |
|-------|-------------|
| `CommandPool` | Command buffer pool |
| `CommandPoolBuilder` | Pool creation |
| `CommandBuffer` | Command buffer |
| `CommandBufferRecorder` | RAII recording wrapper |

## Synchronization

| Class | Description |
|-------|-------------|
| `Fence` | CPU-GPU synchronization |
| `FenceBuilder` | Fence creation |
| `Semaphore` | GPU-GPU synchronization |
| `SemaphoreBuilder` | Semaphore creation |
| `ResourceTracker` | Automatic barrier generation |

## Render Passes

| Class | Description |
|-------|-------------|
| `Subpass<SlotEnum>` | Base class with lazy images |
| `ScreenSpacePass<SlotEnum>` | Fullscreen effect base |
| `SkyPass` | Atmospheric sky rendering |
| `SunLightPass` | Direct sun with RT shadows |
| `IndirectLightPass` | Indirect sky lighting |
| `ToneMappingPass` | HDR to LDR conversion |

## Ray Tracing (vw::rt)

| Class | Description |
|-------|-------------|
| `BottomLevelAccelerationStructure` | Geometry BLAS |
| `TopLevelAccelerationStructure` | Instance TLAS |
| `RayTracingPipeline` | RT pipeline |
| `RayTracingPipelineBuilder` | Pipeline creation |
| `ShaderBindingTable` | SBT management |
| `RayTracedScene` | High-level RT scene |

## Model (vw::Model)

| Class | Description |
|-------|-------------|
| `Mesh` | Geometry data |
| `MeshManager` | Mesh loading |
| `Scene` | Instance collection |
| `MeshInstance` | Mesh + transform |
| `Importer` | Assimp wrapper |

## Materials (vw::Model::Material)

| Class | Description |
|-------|-------------|
| `Material` | Material reference |
| `MaterialTypeTag` | Type identifier |
| `IMaterialTypeHandler` | Handler interface |
| `MaterialTypeHandler<T>` | Handler base |
| `BindlessMaterialManager` | Central manager |
| `BindlessTextureManager` | Texture array manager |
| `ColoredMaterialHandler` | Solid color materials |
| `TexturedMaterialHandler` | PBR textured materials |

## Window

| Class | Description |
|-------|-------------|
| `SDL_Initializer` | RAII SDL init |
| `Window` | SDL window wrapper |
| `WindowBuilder` | Window creation |

## Utilities

| Class | Description |
|-------|-------------|
| `ObjectWithHandle<T>` | Non-owning handle wrapper |
| `ObjectWithUniqueHandle<T>` | Owning handle wrapper |
| `Interval<T>` | Range representation |
| `IntervalSet<T>` | Non-overlapping intervals |

## Exceptions

| Class | Description |
|-------|-------------|
| `Exception` | Base exception |
| `VulkanException` | Vulkan API errors |
| `SDLException` | SDL errors |
| `VMAException` | Allocation errors |
| `FileException` | File I/O errors |
| `AssimpException` | Model loading errors |
| `ShaderCompilationException` | Shader errors |
| `LogicException` | Logic errors |
