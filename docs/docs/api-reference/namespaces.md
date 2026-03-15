---
sidebar_position: 1
---

# Namespaces

VulkanWrapper organizes its API into logical namespaces. This page lists every namespace, its classes, and the key types defined within it.

## `vw` -- Core Namespace

The primary namespace containing all foundational types.

### Vulkan Foundation

| Type | Kind | Header |
|------|------|--------|
| `Instance` | class | `Vulkan/Instance.h` |
| `InstanceBuilder` | class | `Vulkan/Instance.h` |
| `PhysicalDevice` | class | `Vulkan/PhysicalDevice.h` |
| `Device` | class | `Vulkan/Device.h` |
| `DeviceFinder` | class | `Vulkan/DeviceFinder.h` |
| `Queue` | class | `Vulkan/Queue.h` |
| `PresentQueue` | class | `Vulkan/PresentQueue.h` |
| `Surface` | class | `Vulkan/Surface.h` |
| `Swapchain` | class | `Vulkan/Swapchain.h` |
| `SwapchainBuilder` | class | `Vulkan/Swapchain.h` |

### Memory Management

| Type | Kind | Header |
|------|------|--------|
| `Allocator` | class | `Memory/Allocator.h` |
| `AllocatorBuilder` | class | `Memory/Allocator.h` |
| `BufferBase` | class | `Memory/Buffer.h` |
| `Buffer<T, HostVisible, Flags>` | class template | `Memory/Buffer.h` |
| `BufferList` | class | `Memory/BufferList.h` |
| `StagingBufferManager` | class | `Memory/StagingBufferManager.h` |
| `Transfer` | class | `Memory/Transfer.h` |
| `UniformBufferAllocator` | class | `Memory/UniformBufferAllocator.h` |

### Image Handling

| Type | Kind | Header |
|------|------|--------|
| `Image` | class | `Image/Image.h` |
| `ImageView` | class | `Image/ImageView.h` |
| `ImageViewBuilder` | class | `Image/ImageView.h` |
| `Sampler` | class | `Image/Sampler.h` |
| `SamplerBuilder` | class | `Image/Sampler.h` |
| `CombinedImage` | class | `Image/CombinedImage.h` |
| `Mipmap` | class | `Image/Mipmap.h` |
| `ImageLoader` | class | `Image/ImageLoader.h` |

### Pipeline

| Type | Kind | Header |
|------|------|--------|
| `Pipeline` | class | `Pipeline/Pipeline.h` |
| `GraphicsPipelineBuilder` | class | `Pipeline/Pipeline.h` |
| `PipelineLayout` | class | `Pipeline/PipelineLayout.h` |
| `PipelineLayoutBuilder` | class | `Pipeline/PipelineLayout.h` |
| `ShaderModule` | class | `Pipeline/ShaderModule.h` |
| `ComputePipelineBuilder` | class | `Pipeline/ComputePipeline.h` |
| `ColorBlendConfig` | struct | `Pipeline/Pipeline.h` |
| `MeshRenderer` | class | `Pipeline/MeshRenderer.h` |

### Descriptors

| Type | Kind | Header |
|------|------|--------|
| `DescriptorSet` | class | `Descriptors/DescriptorSet.h` |
| `DescriptorSetLayout` | class | `Descriptors/DescriptorSetLayout.h` |
| `DescriptorSetLayoutBuilder` | class | `Descriptors/DescriptorSetLayout.h` |
| `DescriptorPool` | class | `Descriptors/DescriptorPool.h` |
| `DescriptorAllocator` | class | `Descriptors/DescriptorAllocator.h` |

### Vertex Types

| Type | Kind | Header |
|------|------|--------|
| `Vertex` | concept | `Descriptors/Vertex.h` |
| `VertexInterface<Ts...>` | class template | `Descriptors/Vertex.h` |
| `ColoredVertex<Position>` | struct template | `Descriptors/Vertex.h` |
| `ColoredAndTexturedVertex<Position>` | struct template | `Descriptors/Vertex.h` |
| `Vertex3D` | struct | `Descriptors/Vertex.h` |
| `FullVertex3D` | struct | `Descriptors/Vertex.h` |
| `ColoredVertex2D` | type alias | `Descriptors/Vertex.h` |
| `ColoredVertex3D` | type alias | `Descriptors/Vertex.h` |
| `ColoredAndTexturedVertex2D` | type alias | `Descriptors/Vertex.h` |
| `ColoredAndTexturedVertex3D` | type alias | `Descriptors/Vertex.h` |

### Command

| Type | Kind | Header |
|------|------|--------|
| `CommandPool` | class | `Command/CommandPool.h` |
| `CommandPoolBuilder` | class | `Command/CommandPool.h` |
| `CommandBuffer` | class | `Command/CommandBuffer.h` |
| `CommandBufferRecorder` | class | `Command/CommandBuffer.h` |

### Synchronization

| Type | Kind | Header |
|------|------|--------|
| `Fence` | class | `Synchronization/Fence.h` |
| `FenceBuilder` | class | `Synchronization/Fence.h` |
| `Semaphore` | class | `Synchronization/Semaphore.h` |
| `SemaphoreBuilder` | class | `Synchronization/Semaphore.h` |
| `Interval<T>` | class template | `Memory/Interval.h` |
| `IntervalSet<T>` | class template | `Memory/IntervalSet.h` |

### Shader

| Type | Kind | Header |
|------|------|--------|
| `ShaderCompiler` | class | `Shader/ShaderCompiler.h` |

### Render Passes

| Type | Kind | Header |
|------|------|--------|
| `RenderPass` | class (abstract) | `RenderPass/RenderPass.h` |
| `ScreenSpacePass` | class | `RenderPass/ScreenSpacePass.h` |
| `RenderPipeline` | class | `RenderPass/RenderPipeline.h` |
| `ValidationResult` | struct | `RenderPass/RenderPipeline.h` |
| `Slot` | enum class | `RenderPass/Slot.h` |
| `CachedImage` | struct | `RenderPass/RenderPass.h` |
| `ZPass` | class | `RenderPass/ZPass.h` |
| `DirectLightPass` | class | `RenderPass/DirectLightPass.h` |
| `AmbientOcclusionPass` | class | `RenderPass/AmbientOcclusionPass.h` |
| `SkyPass` | class | `RenderPass/SkyPass.h` |
| `IndirectLightPass` | class | `RenderPass/IndirectLightPass.h` |
| `ToneMappingPass` | class | `RenderPass/ToneMappingPass.h` |
| `ToneMappingOperator` | enum class | `RenderPass/ToneMappingPass.h` |
| `SkyParameters` | struct | `RenderPass/SkyParameters.h` |
| `SkyParametersGPU` | struct | `RenderPass/SkyParameters.h` |

### Random

| Type | Kind | Header |
|------|------|--------|
| `NoiseTexture` | class | `Random/NoiseTexture.h` |
| `RandomSamplingBuffer` | class | `Random/RandomSamplingBuffer.h` |

### Window

| Type | Kind | Header |
|------|------|--------|
| `SDL_Initializer` | class | `Window/SDL_Initializer.h` |
| `Window` | class | `Window/Window.h` |
| `WindowBuilder` | class | `Window/Window.h` |

### Utilities

| Type | Kind | Header |
|------|------|--------|
| `ObjectWithHandle<T>` | class template | `Utils/ObjectWithHandle.h` |
| `ObjectWithUniqueHandle<T>` | type alias | `Utils/ObjectWithHandle.h` |
| `to_handle` | range adaptor | `Utils/ObjectWithHandle.h` |

### Exceptions

| Type | Base | Header |
|------|------|--------|
| `Exception` | `std::exception` | `Utils/Error.h` |
| `VulkanException` | `Exception` | `Utils/Error.h` |
| `VMAException` | `Exception` | `Utils/Error.h` |
| `SDLException` | `Exception` | `Utils/Error.h` |
| `FileException` | `Exception` | `Utils/Error.h` |
| `AssimpException` | `Exception` | `Utils/Error.h` |
| `ShaderCompilationException` | `Exception` | `Utils/Error.h` |
| `ValidationLayerException` | `Exception` | `Utils/Error.h` |
| `SwapchainException` | `Exception` | `Utils/Error.h` |
| `LogicException` | `Exception` | `Utils/Error.h` |

---

## `vw::Barrier` -- Synchronization Barriers

Automatic pipeline barrier management using Vulkan Synchronization2.

| Type | Kind | Header |
|------|------|--------|
| `ResourceTracker` | class | `Synchronization/ResourceTracker.h` |
| `ImageState` | struct | `Synchronization/ResourceTracker.h` |
| `BufferState` | struct | `Synchronization/ResourceTracker.h` |
| `AccelerationStructureState` | struct | `Synchronization/ResourceTracker.h` |
| `ResourceState` | type alias (`std::variant`) | `Synchronization/ResourceTracker.h` |

```cpp
// ResourceState is a variant of the three state types:
using ResourceState = std::variant<ImageState, BufferState, AccelerationStructureState>;
```

---

## `vw::Model` -- 3D Models

Mesh loading, scene management, and geometric primitives.

| Type | Kind | Header |
|------|------|--------|
| `Scene` | class | `Model/Scene.h` |
| `MeshInstance` | struct | `Model/Scene.h` |
| `Mesh` | class | `Model/Mesh.h` |
| `MeshPushConstants` | struct | `Model/Mesh.h` |
| `MeshManager` | class | `Model/MeshManager.h` |
| `Importer` | class | `Model/Importer.h` |
| `PlanePrimitive` | enum class | `Model/Primitive.h` |
| `ModelPrimitiveResult` | struct | `Model/Primitive.h` |

### Primitive Factory Functions

```cpp
ModelPrimitiveResult create_triangle(PlanePrimitive plane);
ModelPrimitiveResult create_square(PlanePrimitive plane);
ModelPrimitiveResult create_cube();
```

---

## `vw::Model::Material` -- Material System

Bindless material management with type-tagged material handlers.

| Type | Kind | Header |
|------|------|--------|
| `Material` | class | `Material/Material.h` |
| `MaterialTypeTag` | struct | `Material/MaterialTypeTag.h` |
| `MaterialPriority` | enum class | `Material/MaterialPriority.h` |
| `IMaterialTypeHandler` | class (interface) | `Material/IMaterialTypeHandler.h` |
| `MaterialTypeHandler<GpuData>` | class template | `Material/MaterialTypeHandler.h` |
| `ColoredMaterialHandler` | class | `Material/ColoredMaterialHandler.h` |
| `TexturedMaterialHandler` | class | `Material/TexturedMaterialHandler.h` |
| `EmissiveTexturedMaterialHandler` | class | `Material/EmissiveTexturedMaterialHandler.h` |
| `BindlessMaterialManager` | class | `Material/BindlessMaterialManager.h` |
| `BindlessTextureManager` | class | `Material/BindlessTextureManager.h` |

---

## `vw::rt` -- Ray Tracing

High-level ray tracing pipeline, scene management, and shader binding tables.

| Type | Kind | Header |
|------|------|--------|
| `RayTracedScene` | class | `RayTracing/RayTracedScene.h` |
| `InstanceId` | struct | `RayTracing/RayTracedScene.h` |
| `RayTracingPipeline` | class | `RayTracing/RayTracingPipeline.h` |
| `RayTracingPipelineBuilder` | class | `RayTracing/RayTracingPipeline.h` |
| `ShaderBindingTable` | class | `RayTracing/ShaderBindingTable.h` |
| `ShaderBindingTableRecord` | struct | `RayTracing/ShaderBindingTable.h` |
| `GeometryReference` | struct | `RayTracing/GeometryReference.h` |
| `GeometryReferenceBuffer` | type alias | `RayTracing/GeometryReference.h` |

---

## `vw::rt::as` -- Acceleration Structures

Low-level BLAS/TLAS building and management.

| Type | Kind | Header |
|------|------|--------|
| `BottomLevelAccelerationStructure` | class | `RayTracing/BottomLevelAccelerationStructure.h` |
| `BottomLevelAccelerationStructureList` | class | `RayTracing/BottomLevelAccelerationStructure.h` |
| `BottomLevelAccelerationStructureBuilder` | class | `RayTracing/BottomLevelAccelerationStructure.h` |
| `TopLevelAccelerationStructure` | class | `RayTracing/TopLevelAccelerationStructure.h` |
| `TopLevelAccelerationStructureBuilder` | class | `RayTracing/TopLevelAccelerationStructure.h` |

---

## `vw::tests` -- Testing Utilities

Shared GPU singleton for Google Test suites.

| Type | Kind | Header |
|------|------|--------|
| `GPU` | struct | `tests/utils/create_gpu.hpp` |
| `create_gpu()` | function | `tests/utils/create_gpu.hpp` |

```cpp
auto& gpu = vw::tests::create_gpu();
// gpu.instance, gpu.device, gpu.allocator, gpu.queue()
```

---

## Strong Types

Defined in the `vw` namespace, these prevent accidental argument swaps at compile time.

| Type | Wraps | Purpose |
|------|-------|---------|
| `Width` | `uint32_t` | Image/viewport width |
| `Height` | `uint32_t` | Image/viewport height |
| `Depth` | `uint32_t` | Image depth |
| `MipLevel` | `uint32_t` | Mipmap level index |
| `ArrayLayer` | `uint32_t` | Image array layer index |

```cpp
// Strong types prevent argument order mistakes
allocator->create_image_2D(Width{1920}, Height{1080}, ...);
// allocator->create_image_2D(1920, 1080, ...);  // Won't compile
```

---

## Enums

### `ApiVersion`

```cpp
enum class ApiVersion { e10, e11, e12, e13 };
```

### `Slot` (Render Pass Image Slots)

```cpp
enum class Slot {
    Depth, Albedo, Normal, Tangent, Bitangent, Position,
    DirectLight, IndirectRay, AmbientOcclusion, Sky,
    IndirectLight, ToneMapped,
    UserSlot = 1024  // Extension point
};
```

### `ToneMappingOperator`

```cpp
enum class ToneMappingOperator : int32_t {
    ACES = 0, Reinhard = 1, ReinhardExtended = 2,
    Uncharted2 = 3, Neutral = 4
};
```

---

## Buffer Usage Constants

Predefined `VkBufferUsageFlags2` constants for common buffer types. Each includes `eTransferDst` and `eShaderDeviceAddress` automatically.

| Constant | Primary Usage Flag | Extra Flags |
|----------|-------------------|-------------|
| `VertexBufferUsage` | `eVertexBuffer` | `eAccelerationStructureBuildInputReadOnlyKHR` |
| `IndexBufferUsage` | `eIndexBuffer` | `eAccelerationStructureBuildInputReadOnlyKHR` |
| `UniformBufferUsage` | `eUniformBuffer` | -- |
| `StorageBufferUsage` | `eStorageBuffer` | -- |
| `StagingBufferUsage` | `eTransferSrc` | -- |

Defined in `Memory/BufferUsage.h`.

---

## Free Functions

### Error Checking

```cpp
auto result = check_vk(vulkan_result, "context message");  // throws VulkanException
check_vma(vma_result, "context message");                   // throws VMAException
check_sdl(pointer_or_bool, "context message");              // throws SDLException
```

### Buffer Factories

```cpp
// Generic buffer creation
auto buf = create_buffer<float, true, UniformBufferUsage>(allocator, count);

// Vertex buffer shortcut
auto vbuf = allocate_vertex_buffer<FullVertex3D, false>(allocator, count);

// Create from Buffer type alias
auto buf2 = create_buffer<MyBufferType>(allocator, count);
```

### Pipeline Factory

```cpp
auto pipeline = create_screen_space_pipeline(
    device, vertex_shader, fragment_shader,
    descriptor_set_layout, color_format,
    depth_format,        // optional, default: eUndefined
    depth_compare_op,    // optional, default: eAlways
    push_constants,      // optional, default: {}
    blend                // optional, default: nullopt
);
```

---

## Range Adaptors

### `vw::to_handle`

Extracts Vulkan handles from wrapper objects in a range pipeline.

```cpp
auto handles = objects | vw::to_handle | std::ranges::to<std::vector>();
```

Works with both value types (`x.handle()`) and pointer types (`x->handle()`).
