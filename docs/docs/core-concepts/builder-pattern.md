---
sidebar_position: 2
---

# Builder Pattern

VulkanWrapper uses the builder pattern extensively for constructing complex objects with many optional parameters. This provides a fluent, readable API while maintaining type safety.

## Why Builders?

Vulkan objects often require many configuration parameters, most of which have sensible defaults. The builder pattern offers several advantages:

1. **Fluent API**: Chain method calls for readable construction
2. **Optional parameters**: Only specify what you need
3. **Compile-time validation**: Invalid configurations caught at compile time
4. **Self-documenting**: Method names describe what each option does

## Basic Pattern

Every builder follows this structure:

```cpp
class SomeObjectBuilder {
public:
    // Constructor takes required dependencies
    SomeObjectBuilder(std::shared_ptr<const Device> device);

    // Configuration methods return *this for chaining
    SomeObjectBuilder& with_some_option();
    SomeObjectBuilder& set_some_value(int value);

    // build() creates the final object
    SomeObject build();
};
```

## Usage Example

Here's how to use the `CommandPoolBuilder`:

```cpp
// Create a command pool with default settings
auto pool = CommandPoolBuilder(device).build();

// Create a command pool with reset capability
auto resettablePool = CommandPoolBuilder(device)
    .with_reset_command_buffer()
    .build();
```

## Available Builders

VulkanWrapper provides builders for most major objects:

### Core Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `InstanceBuilder` | `Instance` | Debug layers, extensions |
| `DeviceFinder` | `Device` | Queue requirements, features |
| `AllocatorBuilder` | `Allocator` | VMA configuration |
| `SwapchainBuilder` | `Swapchain` | Present mode, format |

### Pipeline Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `GraphicsPipelineBuilder` | `Pipeline` | Shaders, vertex input, blend |
| `PipelineLayoutBuilder` | `PipelineLayout` | Descriptor layouts, push constants |
| `RayTracingPipelineBuilder` | `RayTracingPipeline` | Shader groups, recursion depth |

### Resource Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `ImageViewBuilder` | `ImageView` | Aspect, mip levels |
| `SamplerBuilder` | `Sampler` | Filtering, addressing |
| `CommandPoolBuilder` | `CommandPool` | Reset flags |

### Synchronization Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `FenceBuilder` | `Fence` | Signaled state |
| `SemaphoreBuilder` | `Semaphore` | Timeline vs binary |

### Ray Tracing Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `BottomLevelAccelerationStructureBuilder` | `BLAS` | Geometry, build flags |
| `TopLevelAccelerationStructureBuilder` | `TLAS` | Instances, build flags |

### Window Objects

| Builder | Creates | Key Options |
|---------|---------|-------------|
| `WindowBuilder` | `Window` | Size, title, flags |

## Design Principles

### Private Constructors

Objects created by builders have private constructors, ensuring they can only be created through the builder:

```cpp
class CommandPool {
    friend class CommandPoolBuilder;
private:
    CommandPool(/* params */);  // Only builder can construct
};
```

### Method Chaining

Builder methods return a reference to themselves, enabling fluent chaining:

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .with_vertex_shader(vertShader)
    .with_fragment_shader(fragShader)
    .with_vertex_type<FullVertex3D>()
    .with_depth_test()
    .build();
```

### Required vs Optional

- **Required parameters** are passed to the builder constructor
- **Optional parameters** are set via `with_*()` or `set_*()` methods
- Sensible defaults are used for unspecified options
