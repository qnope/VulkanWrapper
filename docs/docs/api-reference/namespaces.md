---
sidebar_position: 1
---

# Namespaces

VulkanWrapper organizes code into logical namespaces.

## Main Namespace

### `vw`

The primary namespace containing all core types:

```cpp
namespace vw {
    // Vulkan foundation
    class Instance;
    class Device;
    class PhysicalDevice;
    class Queue;
    class PresentQueue;
    class Surface;
    class Swapchain;

    // Memory management
    class Allocator;
    class BufferBase;
    template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
    class Buffer;
    class BufferList;
    class StagingBufferManager;
    class UniformBufferAllocator;

    // Image handling
    class Image;
    class ImageView;
    class Sampler;
    class CombinedImage;

    // Pipeline
    class Pipeline;
    class PipelineLayout;
    class ShaderModule;

    // Descriptors
    class DescriptorSetLayout;
    class DescriptorPool;
    class DescriptorSet;
    class DescriptorAllocator;

    // Command
    class CommandPool;
    class CommandBuffer;
    class CommandBufferRecorder;

    // Synchronization
    class Fence;
    class Semaphore;
    class ResourceTracker;

    // Render passes
    template <typename SlotEnum>
    class Subpass;
    template <typename SlotEnum>
    class ScreenSpacePass;
    class SkyPass;
    class SunLightPass;
    class SkyLightPass;
    class ToneMappingPass;

    // Window
    class SDL_Initializer;
    class Window;

    // Utilities
    class Exception;
    class VulkanException;
    class SDLException;
    // ... other exception types

    // Builders
    class InstanceBuilder;
    class DeviceFinder;
    class AllocatorBuilder;
    class GraphicsPipelineBuilder;
    class PipelineLayoutBuilder;
    class DescriptorSetLayoutBuilder;
    class DescriptorPoolBuilder;
    class ImageBuilder;
    class ImageViewBuilder;
    class SamplerBuilder;
    class SwapchainBuilder;
    class CommandPoolBuilder;
    class FenceBuilder;
    class SemaphoreBuilder;
    class WindowBuilder;
}
```

## Sub-Namespaces

### `vw::Barrier`

Barrier-related utilities and types:

```cpp
namespace vw::Barrier {
    struct ImageState;
    struct BufferState;
}
```

### `vw::Model`

3D model and mesh types:

```cpp
namespace vw::Model {
    class Mesh;
    class MeshManager;
    class Scene;
    struct MeshInstance;
    class Importer;
}
```

### `vw::Model::Material`

Material system types:

```cpp
namespace vw::Model::Material {
    struct Material;
    struct MaterialTypeTag;
    enum class MaterialPriority;

    class IMaterialTypeHandler;
    template <typename DataT>
    class MaterialTypeHandler;

    class BindlessMaterialManager;
    class BindlessTextureManager;
    class ColoredMaterialHandler;
    class TexturedMaterialHandler;
}
```

### `vw::rt`

Ray tracing types:

```cpp
namespace vw::rt {
    class BottomLevelAccelerationStructure;
    class TopLevelAccelerationStructure;
    class RayTracingPipeline;
    class RayTracingPipelineBuilder;
    class ShaderBindingTable;
    class RayTracedScene;
}
```

## Type Aliases

Common type aliases defined in `vw`:

```cpp
namespace vw {
    // Strong types
    struct Width { uint32_t value; };
    struct Height { uint32_t value; };
    struct Depth { uint32_t value; };
    struct MipLevel { uint32_t value; };
    struct ArrayLayer { uint32_t value; };

    // API version
    enum class ApiVersion { e10, e11, e12, e13 };

    // Buffer usage constants
    constexpr VkBufferUsageFlags VertexBufferUsage = /* ... */;
    constexpr VkBufferUsageFlags IndexBufferUsage = /* ... */;
    constexpr VkBufferUsageFlags UniformBufferUsage = /* ... */;
    constexpr VkBufferUsageFlags StorageBufferUsage = /* ... */;
    constexpr VkBufferUsageFlags TransferSrcUsage = /* ... */;
    constexpr VkBufferUsageFlags TransferDstUsage = /* ... */;
}
```

## Importing

Recommended import patterns:

```cpp
// Import entire namespace (for application code)
using namespace vw;

// Or use explicit namespace
auto instance = vw::InstanceBuilder().build();

// For material system
using namespace vw::Model::Material;
auto material = manager.create_colored({1, 0, 0, 1});

// For ray tracing
namespace rt = vw::rt;
auto blas = rt::BottomLevelAccelerationStructure::build(/* ... */);
```
