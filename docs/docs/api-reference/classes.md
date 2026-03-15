---
sidebar_position: 2
---

# Classes

Quick-reference tables for every class in VulkanWrapper, organized by module. All header paths are relative to `VulkanWrapper/include/VulkanWrapper/`.

## Vulkan Foundation

| Class | Description | Header |
|-------|-------------|--------|
| `Instance` | Vulkan instance with debug/validation layer support | `Vulkan/Instance.h` |
| `InstanceBuilder` | Fluent builder for `Instance` | `Vulkan/Instance.h` |
| `PhysicalDevice` | Physical GPU properties and feature queries | `Vulkan/PhysicalDevice.h` |
| `Device` | Logical device with queue families | `Vulkan/Device.h` |
| `DeviceFinder` | GPU selection with feature requirements (returned by `Instance::findGpu()`) | `Vulkan/DeviceFinder.h` |
| `Queue` | Command submission queue | `Vulkan/Queue.h` |
| `PresentQueue` | Queue with presentation support | `Vulkan/PresentQueue.h` |
| `Surface` | Window surface for presentation | `Vulkan/Surface.h` |
| `Swapchain` | Presentation swapchain with image views | `Vulkan/Swapchain.h` |
| `SwapchainBuilder` | Fluent builder for `Swapchain` | `Vulkan/Swapchain.h` |

## Memory Management

| Class | Description | Header |
|-------|-------------|--------|
| `Allocator` | VMA-based GPU memory allocator; creates images and buffers | `Memory/Allocator.h` |
| `AllocatorBuilder` | Fluent builder for `Allocator` | `Memory/Allocator.h` |
| `BufferBase` | Non-templated base class for GPU buffers | `Memory/Buffer.h` |
| `Buffer<T, HostVisible, Flags>` | Type-safe GPU buffer with compile-time usage and visibility | `Memory/Buffer.h` |
| `BufferList` | Dynamic buffer pool that grows by allocating new buffers | `Memory/BufferList.h` |
| `StagingBufferManager` | Batches CPU-to-GPU data uploads via staging buffers | `Memory/StagingBufferManager.h` |
| `Transfer` | Image/buffer copy operations with automatic barrier management | `Memory/Transfer.h` |
| `UniformBufferAllocator` | Sub-allocates aligned uniform data within a single buffer | `Memory/UniformBufferAllocator.h` |

## Image Handling

| Class | Description | Header |
|-------|-------------|--------|
| `Image` | Vulkan image wrapper (no VMA dependency) | `Image/Image.h` |
| `ImageView` | View into an image (color, depth, specific mip/layer) | `Image/ImageView.h` |
| `ImageViewBuilder` | Fluent builder for `ImageView` | `Image/ImageView.h` |
| `Sampler` | Texture sampler (filtering, addressing, anisotropy) | `Image/Sampler.h` |
| `SamplerBuilder` | Fluent builder for `Sampler` | `Image/Sampler.h` |
| `CombinedImage` | Pairs an `ImageView` with a `Sampler` for descriptor binding | `Image/CombinedImage.h` |
| `Mipmap` | Generates mipmaps for an image via blit chains | `Image/Mipmap.h` |
| `ImageLoader` | Loads images from disk (PNG, JPG, BMP) into GPU images | `Image/ImageLoader.h` |

## Pipeline

| Class | Description | Header |
|-------|-------------|--------|
| `Pipeline` | Owns a `vk::UniquePipeline` and its associated `PipelineLayout` | `Pipeline/Pipeline.h` |
| `GraphicsPipelineBuilder` | Fluent builder for graphics pipelines (vertex input, shaders, blending, depth) | `Pipeline/Pipeline.h` |
| `PipelineLayout` | Descriptor set layouts and push constant ranges | `Pipeline/PipelineLayout.h` |
| `PipelineLayoutBuilder` | Fluent builder for `PipelineLayout` | `Pipeline/PipelineLayout.h` |
| `ShaderModule` | SPIR-V shader module wrapper | `Pipeline/ShaderModule.h` |
| `ComputePipelineBuilder` | Fluent builder for compute pipelines | `Pipeline/ComputePipeline.h` |
| `ColorBlendConfig` | Blend factor configuration with static presets (`alpha()`, `additive()`, `constant_blend()`) | `Pipeline/Pipeline.h` |
| `MeshRenderer` | Binds pipelines per `MaterialTypeTag` for bindless mesh rendering | `Pipeline/MeshRenderer.h` |

## Descriptors

| Class | Description | Header |
|-------|-------------|--------|
| `DescriptorSet` | Allocated descriptor set for shader resource binding | `Descriptors/DescriptorSet.h` |
| `DescriptorSetLayout` | Defines the binding layout for a descriptor set | `Descriptors/DescriptorSetLayout.h` |
| `DescriptorSetLayoutBuilder` | Fluent builder for `DescriptorSetLayout` | `Descriptors/DescriptorSetLayout.h` |
| `DescriptorPool` | Pool from which descriptor sets are allocated | `Descriptors/DescriptorPool.h` |
| `DescriptorAllocator` | High-level descriptor management with automatic pool growth | `Descriptors/DescriptorAllocator.h` |

## Vertex Types

| Class | Description | Header |
|-------|-------------|--------|
| `VertexInterface<Ts...>` | CRTP base that auto-generates `binding_description` and `attribute_descriptions` from member types | `Descriptors/Vertex.h` |
| `Vertex3D` | Position-only vertex (`vec3`) | `Descriptors/Vertex.h` |
| `ColoredVertex<Position>` | Position + color (`vec3`) vertex | `Descriptors/Vertex.h` |
| `ColoredAndTexturedVertex<Position>` | Position + color + UV vertex | `Descriptors/Vertex.h` |
| `FullVertex3D` | Position + normal + tangent + bitangent + UV vertex | `Descriptors/Vertex.h` |

**Type aliases:** `ColoredVertex2D`, `ColoredVertex3D`, `ColoredAndTexturedVertex2D`, `ColoredAndTexturedVertex3D`

## Command

| Class | Description | Header |
|-------|-------------|--------|
| `CommandPool` | Pool for allocating command buffers | `Command/CommandPool.h` |
| `CommandPoolBuilder` | Fluent builder for `CommandPool` | `Command/CommandPool.h` |
| `CommandBuffer` | Records GPU commands (draw, dispatch, copy, barriers) | `Command/CommandBuffer.h` |
| `CommandBufferRecorder` | RAII wrapper that begins recording on construction and ends on destruction | `Command/CommandBuffer.h` |

## Synchronization

| Class | Description | Header |
|-------|-------------|--------|
| `Fence` | CPU-GPU synchronization primitive | `Synchronization/Fence.h` |
| `FenceBuilder` | Fluent builder for `Fence` | `Synchronization/Fence.h` |
| `Semaphore` | GPU-GPU synchronization primitive | `Synchronization/Semaphore.h` |
| `SemaphoreBuilder` | Fluent builder for `Semaphore` | `Synchronization/Semaphore.h` |
| `Interval<T>` | Represents a closed range `[start, end)` | `Memory/Interval.h` |
| `IntervalSet<T>` | Collection of non-overlapping intervals with merge/subtract | `Memory/IntervalSet.h` |

## Synchronization -- `vw::Barrier`

| Class | Description | Header |
|-------|-------------|--------|
| `ResourceTracker` | Tracks resource states and generates optimal pipeline barriers automatically | `Synchronization/ResourceTracker.h` |
| `ImageState` | Tracks an image's layout, pipeline stage, and access flags | `Synchronization/ResourceTracker.h` |
| `BufferState` | Tracks a buffer region's pipeline stage and access flags | `Synchronization/ResourceTracker.h` |
| `AccelerationStructureState` | Tracks an acceleration structure's pipeline stage and access flags | `Synchronization/ResourceTracker.h` |

## Shader

| Class | Description | Header |
|-------|-------------|--------|
| `ShaderCompiler` | Compiles GLSL source/files to SPIR-V shader modules via shaderc | `Shader/ShaderCompiler.h` |

## Render Passes

| Class | Description | Header |
|-------|-------------|--------|
| `RenderPass` | Abstract base class with lazy image allocation via `get_or_create_image()` | `RenderPass/RenderPass.h` |
| `ScreenSpacePass` | Base class for fullscreen post-processing passes; provides `render_fullscreen()` | `RenderPass/ScreenSpacePass.h` |
| `RenderPipeline` | Orchestrates multiple `RenderPass` instances, wiring outputs to inputs by `Slot` | `RenderPass/RenderPipeline.h` |
| `ZPass` | Depth-only pre-pass for early-Z optimization | `RenderPass/ZPass.h` |
| `DirectLightPass` | Sun/direct light with ray-traced shadows | `RenderPass/DirectLightPass.h` |
| `AmbientOcclusionPass` | Screen-space or ray-traced ambient occlusion | `RenderPass/AmbientOcclusionPass.h` |
| `SkyPass` | Physically-based atmospheric sky rendering | `RenderPass/SkyPass.h` |
| `IndirectLightPass` | Indirect sky lighting (bounce light) | `RenderPass/IndirectLightPass.h` |
| `ToneMappingPass` | HDR-to-LDR conversion with selectable tone mapping operators | `RenderPass/ToneMappingPass.h` |
| `SkyParameters` | Physical sky and star parameters for atmospheric rendering (fits in push constants) | `RenderPass/SkyParameters.h` |
| `SkyParametersGPU` | GPU-aligned version of `SkyParameters` (96 bytes, `vec4` members) | `RenderPass/SkyParameters.h` |
| `CachedImage` | Struct pairing a cached `Image` with its `ImageView` | `RenderPass/RenderPass.h` |
| `ValidationResult` | Result of `RenderPipeline::validate()` with error messages | `RenderPass/RenderPipeline.h` |

## Random

| Class | Description | Header |
|-------|-------------|--------|
| `NoiseTexture` | Generates noise textures for sampling in shaders | `Random/NoiseTexture.h` |
| `RandomSamplingBuffer` | GPU buffer of random samples for stochastic rendering | `Random/RandomSamplingBuffer.h` |

## Ray Tracing -- `vw::rt`

| Class | Description | Header |
|-------|-------------|--------|
| `RayTracedScene` | High-level RT scene: manages BLAS/TLAS, geometry references, and an embedded `Scene` | `RayTracing/RayTracedScene.h` |
| `InstanceId` | Opaque identifier for a ray tracing instance | `RayTracing/RayTracedScene.h` |
| `RayTracingPipeline` | Ray tracing pipeline with shader group handles | `RayTracing/RayTracingPipeline.h` |
| `RayTracingPipelineBuilder` | Fluent builder for ray tracing pipelines (raygen, miss, closest-hit shaders) | `RayTracing/RayTracingPipeline.h` |
| `ShaderBindingTable` | Manages raygen, miss, and hit shader binding table records | `RayTracing/ShaderBindingTable.h` |
| `ShaderBindingTableRecord` | Single SBT entry (handle + optional data, 256-byte aligned) | `RayTracing/ShaderBindingTable.h` |
| `GeometryReference` | Per-geometry GPU data for ray tracing shaders (buffer addresses, material, matrix) | `RayTracing/GeometryReference.h` |
| `GeometryReferenceBuffer` | Type alias: `Buffer<GeometryReference, true, StorageBufferUsage>` | `RayTracing/GeometryReference.h` |

## Acceleration Structures -- `vw::rt::as`

| Class | Description | Header |
|-------|-------------|--------|
| `BottomLevelAccelerationStructure` | Geometry-level BLAS with device address | `RayTracing/BottomLevelAccelerationStructure.h` |
| `BottomLevelAccelerationStructureList` | Manages a collection of BLAS instances with shared memory pools | `RayTracing/BottomLevelAccelerationStructure.h` |
| `BottomLevelAccelerationStructureBuilder` | Fluent builder for BLAS (add geometry or mesh, then `build_into()`) | `RayTracing/BottomLevelAccelerationStructure.h` |
| `TopLevelAccelerationStructure` | Instance-level TLAS with device address | `RayTracing/TopLevelAccelerationStructure.h` |
| `TopLevelAccelerationStructureBuilder` | Fluent builder for TLAS (add BLAS addresses + transforms, then `build()`) | `RayTracing/TopLevelAccelerationStructure.h` |

## Model -- `vw::Model`

| Class | Description | Header |
|-------|-------------|--------|
| `Scene` | Collection of `MeshInstance` objects for rendering | `Model/Scene.h` |
| `MeshInstance` | A `Mesh` paired with a `glm::mat4` transform | `Model/Scene.h` |
| `Mesh` | Geometry data: vertex/index buffers, material, draw commands, and acceleration structure info | `Model/Mesh.h` |
| `MeshPushConstants` | Push constants for mesh rendering (transform + material buffer address) | `Model/Mesh.h` |
| `MeshManager` | Loads and caches meshes from model files | `Model/MeshManager.h` |
| `Importer` | Assimp-based model file importer | `Model/Importer.h` |

### Primitives -- `vw::Model`

| Function / Type | Description | Header |
|-----------------|-------------|--------|
| `create_triangle(PlanePrimitive)` | Unit equilateral triangle (4 verts) | `Model/Primitive.h` |
| `create_square(PlanePrimitive)` | Unit square (4 verts, 6 indices) | `Model/Primitive.h` |
| `create_cube()` | Unit cube (24 verts, 36 indices) with normals/tangents/UVs | `Model/Primitive.h` |
| `PlanePrimitive` | Enum: `XY`, `XZ`, `YZ` | `Model/Primitive.h` |
| `ModelPrimitiveResult` | Struct: `vertices` + `indices` | `Model/Primitive.h` |

## Materials -- `vw::Model::Material`

| Class | Description | Header |
|-------|-------------|--------|
| `Material` | Material reference holding a type tag and GPU data address | `Model/Material/Material.h` |
| `MaterialTypeTag` | Type identifier for distinguishing material kinds at runtime | `Model/Material/MaterialTypeTag.h` |
| `MaterialPriority` | Enum controlling material render order | `Model/Material/MaterialPriority.h` |
| `IMaterialTypeHandler` | Abstract interface for material type handlers | `Model/Material/IMaterialTypeHandler.h` |
| `MaterialTypeHandler<GpuData>` | Templated base for material handlers with typed GPU data | `Model/Material/MaterialTypeHandler.h` |
| `ColoredMaterialHandler` | Handles solid-color materials | `Model/Material/ColoredMaterialHandler.h` |
| `TexturedMaterialHandler` | Handles PBR textured materials | `Model/Material/TexturedMaterialHandler.h` |
| `EmissiveTexturedMaterialHandler` | Handles emissive textured materials | `Model/Material/EmissiveTexturedMaterialHandler.h` |
| `BindlessMaterialManager` | Central manager for all material types; dispatches to handlers | `Model/Material/BindlessMaterialManager.h` |
| `BindlessTextureManager` | Manages bindless texture arrays for material rendering | `Model/Material/BindlessTextureManager.h` |

## Window

| Class | Description | Header |
|-------|-------------|--------|
| `SDL_Initializer` | RAII initialization/shutdown of SDL3 | `Window/SDL_Initializer.h` |
| `Window` | SDL3 window with Vulkan surface support | `Window/Window.h` |
| `WindowBuilder` | Fluent builder for `Window` | `Window/Window.h` |

## Utilities

| Class | Description | Header |
|-------|-------------|--------|
| `ObjectWithHandle<T>` | Base class wrapping a Vulkan handle (non-owning when `T` is a raw handle) | `Utils/ObjectWithHandle.h` |
| `ObjectWithUniqueHandle<T>` | Type alias for `ObjectWithHandle<T>` where `T` is a `vk::Unique*` (owning, RAII) | `Utils/ObjectWithHandle.h` |

## Exceptions

All exceptions inherit from `vw::Exception`, which inherits from `std::exception`. Defined in `Utils/Error.h`.

| Exception | Thrown By |
|-----------|----------|
| `VulkanException` | `check_vk()` on Vulkan API errors |
| `VMAException` | `check_vma()` on VMA allocation errors |
| `SDLException` | `check_sdl()` on SDL errors |
| `FileException` | File I/O failures |
| `AssimpException` | Model loading failures |
| `ShaderCompilationException` | GLSL-to-SPIR-V compilation errors |
| `ValidationLayerException` | Vulkan validation layer errors |
| `SwapchainException` | Swapchain creation/acquisition errors |
| `LogicException` | Programming logic errors (assertions) |
