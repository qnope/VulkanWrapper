---
sidebar_position: 1
title: "Instance"
---

# Instance

The `Instance` is the entry point to the entire Vulkan API. It represents your application's connection to the Vulkan runtime and is always the first object you create. VulkanWrapper provides `InstanceBuilder` to construct an `Instance` safely using the builder pattern.

## Design Rationale

In raw Vulkan, creating an instance requires filling out a `VkInstanceCreateInfo` structure with arrays of extension names, layer names, and version information. This is error-prone: misspelled extension names cause silent failures, forgetting validation layers makes debugging nearly impossible, and macOS requires a portability flag that is easy to overlook.

`InstanceBuilder` solves these problems by providing a fluent API where each configuration step is a named method call. The builder validates your configuration at build time and returns a `shared_ptr<Instance>`, ensuring the instance lives as long as any object that depends on it.

## API Reference

### InstanceBuilder

```cpp
namespace vw {

class InstanceBuilder {
public:
    InstanceBuilder &addPortability();
    InstanceBuilder &addExtension(const char *extension);
    InstanceBuilder &addExtensions(std::span<const char *const> extensions);
    InstanceBuilder &setDebug();
    InstanceBuilder &setApiVersion(ApiVersion version);

    std::shared_ptr<Instance> build();
};

} // namespace vw
```

**Header:** `VulkanWrapper/Vulkan/Instance.h`

#### `addPortability()`

Enables the Vulkan portability subset extension and associated instance flags. This is **required on macOS**, where the Vulkan implementation (MoltenVK or llvmpipe) is a translation layer over Metal. Without this call, `build()` will fail on macOS because the Vulkan loader cannot enumerate any physical devices.

It is harmless on platforms that do not need it, so you can safely call it unconditionally.

#### `addExtension(const char *extension)`

Adds a single Vulkan instance extension by name. You typically do not need to call this directly unless you need a non-standard extension. Window-related extensions are better handled with `addExtensions()`.

#### `addExtensions(std::span<const char *const> extensions)`

Adds multiple extensions at once. This is the preferred way to pass the window system's required extensions:

```cpp
auto extensions = vw::Window::get_required_instance_extensions();
builder.addExtensions(extensions);
```

#### `setDebug()`

Enables Vulkan validation layers and sets up a debug messenger that prints diagnostics to stderr. When enabled, the Vulkan runtime reports API usage errors, performance warnings, and best-practice violations.

**Always enable this during development.** Validation layers catch mistakes that would otherwise manifest as silent corruption, device loss, or platform-specific crashes. Disable them only for release builds where the performance overhead matters.

When active, you will see messages like:

```
VALIDATION: vkCreateGraphicsPipelines: required parameter
            pCreateInfos[0].pStages[0].module was not valid
```

#### `setApiVersion(ApiVersion version)`

Sets the minimum Vulkan API version your application requires. Available values:

| Value             | Vulkan Version | Key Features                                    |
|-------------------|----------------|-------------------------------------------------|
| `ApiVersion::e10` | 1.0            | Default; maximum compatibility                  |
| `ApiVersion::e11` | 1.1            | Subgroup operations, multiview                  |
| `ApiVersion::e12` | 1.2            | Descriptor indexing, timeline semaphores         |
| `ApiVersion::e13` | 1.3            | Dynamic rendering, synchronization2 (recommended) |

VulkanWrapper uses Vulkan 1.3 features extensively (dynamic rendering via `cmd.beginRendering()`, synchronization2 via `vk::PipelineStageFlagBits2`), so you should typically use `ApiVersion::e13`.

The default is `ApiVersion::e10` if you do not call this method.

#### `build()`

Validates the configuration and creates the Vulkan instance. Returns a `std::shared_ptr<Instance>`. Throws `VulkanException` if instance creation fails (for example, if the requested API version is not supported by the driver).

### Instance

```cpp
namespace vw {

class Instance {
public:
    [[nodiscard]] vk::Instance handle() const noexcept;
    [[nodiscard]] DeviceFinder findGpu() const noexcept;
};

} // namespace vw
```

`Instance` is non-copyable and non-movable. It is always managed through a `shared_ptr` returned by `InstanceBuilder::build()`.

#### `handle()`

Returns the raw `vk::Instance` handle. You rarely need this directly -- it is primarily used internally by other VulkanWrapper classes. One common use is passing it to external libraries that need a Vulkan instance.

#### `findGpu()`

Returns a `DeviceFinder` pre-populated with all physical devices (GPUs) available on the system. Use the `DeviceFinder` fluent API to specify requirements and then call `build()` to create a logical `Device`. See the [Device & GPU Selection](device.md) page for details.

```cpp
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();
```

## Usage Patterns

### Minimal Instance (Offscreen Rendering / Tests)

When you do not need a window or presentation -- for example, in unit tests or headless compute workloads -- you can create a simple instance:

```cpp
auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .addPortability()
    .build();
```

This is the pattern used by the test GPU singleton in `tests/utils/create_gpu.hpp`.

### Windowed Application Instance

When you need to display to a window, first create the SDL initializer, then add the window system's required extensions:

```cpp
auto sdl_init = std::make_shared<vw::SDL_Initializer>();
auto extensions = vw::Window::get_required_instance_extensions();

auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .addPortability()
    .addExtensions(extensions)
    .build();
```

The `SDL_Initializer` must be created before calling `get_required_instance_extensions()` and must outlive all windows.

### Complete Initialization Chain

The instance is the first step in a chain that flows through the rest of VulkanWrapper:

```cpp
// 1. Create instance
auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .addPortability()
    .build();

// 2. Find GPU and create device
auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_synchronization_2()
    .with_dynamic_rendering()
    .build();

// 3. Create memory allocator
auto allocator = vw::AllocatorBuilder(instance, device).build();
```

Each object in this chain holds a `shared_ptr` to its dependencies, so the destruction order is automatic and safe.

## Common Pitfalls

### Forgetting `addPortability()` on macOS

**Symptom:** `build()` throws a `VulkanException` on macOS, or `DeviceFinder` reports zero available GPUs.

**Cause:** macOS Vulkan implementations require the portability subset extension. Without it, the Vulkan loader refuses to enumerate any physical devices.

**Fix:** Always call `addPortability()`. It is a no-op on platforms that do not need it.

### Not calling `setDebug()` during development

**Symptom:** Mysterious crashes, corrupted rendering, or "device lost" errors with no useful diagnostics.

**Cause:** Without validation layers, the Vulkan runtime does not check for API misuse. Errors go undetected until they cause undefined behavior.

**Fix:** Always enable debug mode during development:

```cpp
builder.setDebug();
```

### Using the default `ApiVersion::e10` with VulkanWrapper features

**Symptom:** `DeviceFinder::build()` fails to find a suitable GPU, or you get validation errors about features not being available.

**Cause:** VulkanWrapper relies on Vulkan 1.3 features (dynamic rendering, synchronization2). These are not available under API version 1.0.

**Fix:** Use `ApiVersion::e13`:

```cpp
builder.setApiVersion(vw::ApiVersion::e13);
```

### Destroying the Instance too early

**Symptom:** Crash or validation error when destroying a device, allocator, or other Vulkan object.

**Cause:** The `Instance` must outlive all objects created from it. If the `shared_ptr<Instance>` is released (for example, by going out of scope) before the device or allocator, the Vulkan instance handle becomes invalid while still in use.

**Fix:** `InstanceBuilder::build()` returns a `shared_ptr<Instance>`. As long as you pass this shared pointer to `AllocatorBuilder`, the `DeviceFinder`, and other consumers, reference counting ensures the instance stays alive as long as anything needs it.

## Integration with Other Modules

| Next Step | Method | See |
|-----------|--------|-----|
| GPU selection | `instance->findGpu()` | [Device & GPU Selection](device.md) |
| Memory allocator | `AllocatorBuilder(instance, device)` | [Allocator](../memory/allocator.md) |
| Window surface | `window.create_surface(*instance)` | [Swapchain & Window](swapchain.md) |
