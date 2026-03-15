---
sidebar_position: 2
---

# Builder Pattern

VulkanWrapper uses the builder pattern for every complex object. Instead of filling out large C structs with dozens of fields, you chain readable method calls and let the builder handle the details.

## Why Builders?

In raw Vulkan, creating an instance looks like this:

```cpp
// Raw Vulkan -- verbose, error-prone, hard to read
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = "MyApp";
appInfo.apiVersion = VK_API_VERSION_1_3;

VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledLayerCount = 1;
createInfo.ppEnabledLayerNames = layers;
createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames = extensions.data();

VkInstance instance;
vkCreateInstance(&createInfo, nullptr, &instance);
```

With VulkanWrapper, the same thing becomes:

```cpp
// VulkanWrapper -- concise, safe, self-documenting
auto instance = InstanceBuilder()
    .setDebug()
    .setApiVersion(ApiVersion::e13)
    .build();
```

Builders provide four advantages:

1. **Fluent API** -- chain method calls for readable construction.
2. **Sensible defaults** -- only specify what you need to customize.
3. **Compile-time validation** -- invalid configurations are caught before you run.
4. **Self-documenting** -- method names describe what each option does.

## How Builders Work

Every builder follows the same conventions:

- **Required dependencies** go in the constructor.
- **Configuration methods** return a reference to the builder itself (`*this`) for chaining.
- **`build()`** consumes the configuration and returns the final object -- typically a `shared_ptr` or a value type.

```cpp
// General pattern
auto result = SomeBuilder(required_dep_1, required_dep_2)
    .with_option_a()
    .set_option_b(value)
    .build();
```

## All Available Builders

### InstanceBuilder

Creates a Vulkan instance with optional debug layers and extensions.

```cpp
auto instance = InstanceBuilder()
    .setDebug()                          // enable validation layers
    .setApiVersion(ApiVersion::e13)      // require Vulkan 1.3
    .addPortability()                    // macOS portability subset
    .addExtension(ext)                   // add a single extension
    .addExtensions(extensionSpan)        // add multiple extensions
    .build();                            // -> shared_ptr<Instance>
```

### DeviceFinder

Finds a suitable GPU and creates a logical device. Accessed via `instance->findGpu()`.

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)   // require a graphics queue
    .with_presentation(surface)                  // require presentation support
    .with_synchronization_2()                    // enable Synchronization2
    .with_dynamic_rendering()                    // enable dynamic rendering
    .with_descriptor_indexing()                  // enable descriptor indexing
    .with_scalar_block_layout()                  // enable scalar block layout
    .with_ray_tracing()                          // enable ray tracing extensions
    .build();                                    // -> shared_ptr<Device>
```

### AllocatorBuilder

Creates a VMA-backed memory allocator.

```cpp
auto allocator = AllocatorBuilder(instance, device)
    .build();   // -> shared_ptr<Allocator>
```

### WindowBuilder

Creates an SDL3 window.

```cpp
auto window = WindowBuilder(sdl_init)
    .with_title("My Application")
    .sized(Width{1280}, Height{720})
    .build();   // -> Window
```

### ImageViewBuilder

Creates a view into an image for use in shaders or as a render target.

```cpp
auto view = ImageViewBuilder(device, image)
    .setImageType(vk::ImageViewType::e2D)
    .build();   // -> shared_ptr<const ImageView>
```

### SamplerBuilder

Creates a texture sampler.

```cpp
auto sampler = SamplerBuilder(device)
    .build();   // -> shared_ptr<const Sampler>
```

### CommandPoolBuilder

Creates a command pool for allocating command buffers.

```cpp
auto pool = CommandPoolBuilder(device, queue_family_index)
    .with_reset_command_buffer()    // allow individual buffer resets
    .build();                       // -> CommandPool
```

### FenceBuilder

Creates a CPU-GPU synchronization fence.

```cpp
auto fence = FenceBuilder(device)
    .signaled()     // start in signaled state
    .build();       // -> Fence
```

### SemaphoreBuilder

Creates a GPU-GPU synchronization semaphore.

```cpp
auto semaphore = SemaphoreBuilder(device)
    .build();   // -> Semaphore
```

### PipelineLayoutBuilder

Creates a pipeline layout that describes descriptor set layouts and push constant ranges.

```cpp
auto layout = PipelineLayoutBuilder(device)
    .build();   // -> PipelineLayout (or shared_ptr variant)
```

### GraphicsPipelineBuilder

Creates a full graphics pipeline with shaders, vertex input, blending, and depth configuration.

```cpp
auto pipeline = GraphicsPipelineBuilder(device, pipelineLayout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertModule)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragModule)
    .add_vertex_binding<FullVertex3D>()
    .add_color_attachment(format, ColorBlendConfig::alpha())
    .set_depth_format(depthFormat)
    .with_depth_test(true, vk::CompareOp::eLess)
    .with_topology(vk::PrimitiveTopology::eTriangleList)
    .with_dynamic_viewport_scissor()
    .with_cull_mode(vk::CullModeFlagBits::eBack)
    .build();   // -> shared_ptr<const Pipeline>
```

### RayTracingPipelineBuilder

Creates a ray tracing pipeline with generation, hit, and miss shaders.

```cpp
auto rtPipeline = RayTracingPipelineBuilder(device, allocator, pipelineLayout)
    .set_ray_generation_shader(raygenModule)
    .add_closest_hit_shader(closestHitModule)
    .add_miss_shader(missModule)
    .build();   // -> RayTracingPipeline
```

## Builder Summary Table

| Builder | Constructor Args | Returns |
|---------|-----------------|---------|
| `InstanceBuilder` | *(none)* | `shared_ptr<Instance>` |
| `DeviceFinder` | via `instance->findGpu()` | `shared_ptr<Device>` |
| `AllocatorBuilder` | `instance`, `device` | `shared_ptr<Allocator>` |
| `WindowBuilder` | `sdl_init` | `Window` |
| `ImageViewBuilder` | `device`, `image` | `shared_ptr<const ImageView>` |
| `SamplerBuilder` | `device` | `shared_ptr<const Sampler>` |
| `CommandPoolBuilder` | `device`, `queue_family_index` | `CommandPool` |
| `FenceBuilder` | `device` | `Fence` |
| `SemaphoreBuilder` | `device` | `Semaphore` |
| `PipelineLayoutBuilder` | `device` | pipeline layout |
| `GraphicsPipelineBuilder` | `device`, `pipelineLayout` | `shared_ptr<const Pipeline>` |
| `RayTracingPipelineBuilder` | `device`, `allocator`, `pipelineLayout` | `RayTracingPipeline` |

## Design Conventions

### Required vs. Optional

Required dependencies are always in the constructor. Optional configuration uses `with_*()` or `set_*()` methods. If you call `build()` without setting any options, you get a valid object with sensible defaults.

```cpp
// Minimal -- all defaults
auto pool = CommandPoolBuilder(device, queue_family_index).build();

// Customized -- only override what you need
auto pool = CommandPoolBuilder(device, queue_family_index)
    .with_reset_command_buffer()
    .build();
```

### Private Constructors

Objects created by builders have private constructors. The only way to create them is through the builder, which enforces that all invariants are satisfied before the object exists.

### Method Chaining

Every configuration method returns a reference to the builder (`SomeBuilder&`), except `build()` which returns the final object. This lets you chain calls naturally:

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```
