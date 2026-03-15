---
sidebar_position: 3
---

# RAII and Ownership

Vulkan requires you to manually create and destroy every object in the right order. Miss a `vkDestroy*` call and you leak GPU memory; destroy something too early and you crash. VulkanWrapper eliminates this entire class of bugs by managing resource lifetimes automatically through RAII.

## What RAII Means for Vulkan

RAII (Resource Acquisition Is Initialization) ties a resource's lifetime to a C++ object's lifetime. When the object goes out of scope, its destructor cleans up the resource. In VulkanWrapper:

- Creating an object acquires the Vulkan resource.
- The object's destructor releases the Vulkan resource.
- You never call `vkDestroy*` or `vkFree*` yourself.

```cpp
{
    auto instance = InstanceBuilder()
        .setDebug()
        .setApiVersion(ApiVersion::e13)
        .build();

    auto device = instance->findGpu()
        .with_queue(vk::QueueFlagBits::eGraphics)
        .with_synchronization_2()
        .with_dynamic_rendering()
        .build();

    // Use instance and device...

}  // device destroyed first, then instance -- correct Vulkan order
```

## ObjectWithHandle -- The Handle Wrapper

All VulkanWrapper types that wrap a Vulkan handle inherit from `ObjectWithHandle<T>`. The template parameter determines whether the wrapper owns the handle or merely references it.

### Non-Owning (View)

When `T` is a plain Vulkan handle type, the wrapper does not manage the lifetime:

```cpp
ObjectWithHandle<vk::Pipeline>  // does NOT destroy the pipeline
```

This is used when something else owns the handle and you just need to refer to it.

### Owning (Unique Handle)

When `T` is a `vk::Unique*` type, the wrapper owns the handle and destroys it automatically:

```cpp
ObjectWithHandle<vk::UniquePipeline>  // DOES destroy the pipeline
```

`ObjectWithUniqueHandle<T>` is a type alias for `ObjectWithHandle<T>` -- it is not a separate class. The name simply makes ownership intent clearer at the call site.

## Shared Ownership with shared_ptr

Most VulkanWrapper objects are returned as `shared_ptr` from builders:

```cpp
std::shared_ptr<Instance>          instance;
std::shared_ptr<Device>            device;
std::shared_ptr<Allocator>         allocator;
std::shared_ptr<const Pipeline>    pipeline;
std::shared_ptr<const ImageView>   imageView;
std::shared_ptr<const Sampler>     sampler;
```

Shared ownership is used because Vulkan resources are frequently referenced from multiple places at once. A pipeline might be used by several render passes; an image view might be bound in multiple descriptor sets. With `shared_ptr`, the resource stays alive as long as anyone needs it and is destroyed automatically when the last reference drops.

```cpp
std::shared_ptr<const Pipeline> pipeline;

{
    auto localRef = pipeline;  // reference count: 2
    // use localRef...
}  // reference count back to 1, pipeline still alive

pipeline.reset();  // reference count: 0, pipeline destroyed
```

### const Correctness

Notice that many builders return `shared_ptr<const T>` (e.g., `shared_ptr<const Pipeline>`). Once built, these objects are immutable -- you can share them freely across threads without synchronization.

## Resource Dependency Lifetimes

A critical property of Vulkan is that resources must be destroyed before the objects they depend on. For example, a buffer must be destroyed before its allocator, which must be destroyed before the device.

VulkanWrapper handles this automatically. Objects store `shared_ptr` references to their dependencies:

```cpp
// Conceptually, a Buffer holds:
//   shared_ptr<const Device>    m_device;
//   shared_ptr<const Allocator> m_allocator;
//   VmaAllocation               m_allocation;
```

This guarantees that:

1. The `Device` stays alive as long as any `Buffer` exists.
2. The `Allocator` stays alive as long as any allocation exists.
3. When the last reference to a resource drops, cleanup happens in the correct order.

You never need to think about destruction order -- just let objects go out of scope naturally.

## CommandBufferRecorder -- Scoped RAII

`CommandBufferRecorder` is a scoped RAII type for recording command buffers. Its constructor calls `begin()` and its destructor calls `end()`:

```cpp
{
    CommandBufferRecorder recorder(commandBuffer);
    // Recording has started -- issue GPU commands here

    recorder->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    recorder->draw(3, 1, 0, 0);

}  // Destructor calls vkEndCommandBuffer -- recording is complete
```

This pattern makes it impossible to forget to end recording or to accidentally record commands outside a begin/end pair.

## The to_handle Range Adaptor

When you have a collection of VulkanWrapper objects and need to extract the raw Vulkan handles (e.g., for a Vulkan API call that takes a `vk::Pipeline*`), use the `vw::to_handle` range adaptor:

```cpp
std::vector<SomeWrappedObject> objects = /* ... */;

// Extract raw handles as a vector
auto handles = objects | vw::to_handle | std::ranges::to<std::vector>();
```

This is a zero-cost abstraction -- it simply calls `.handle()` on each element through a `std::views::transform`.

## Value Types

Not every type uses `shared_ptr`. Lightweight types that are cheap to copy are returned by value from their builders:

```cpp
auto pool  = CommandPoolBuilder(device, queue_family_index).build();  // CommandPool
auto fence = FenceBuilder(device).signaled().build();                 // Fence
auto sem   = SemaphoreBuilder(device).build();                        // Semaphore
```

These types manage their own Vulkan handle internally (typically via a `vk::Unique*` member) and are move-only.

## Exception Safety

RAII and exceptions work together naturally. If an exception is thrown partway through initialization, all previously constructed objects are destroyed automatically:

```cpp
void setup() {
    auto buffer1 = create_buffer<float, true, UniformBufferUsage>(*allocator, 100);
    auto buffer2 = create_buffer<float, true, UniformBufferUsage>(*allocator, 200);
    // If buffer2 creation throws, buffer1 is still properly freed
}
```

## Best Practices

1. **Let builders manage lifetimes.** Do not manually call Vulkan destroy functions.
2. **Store `shared_ptr` for long-lived, shared resources** (pipelines, image views, samplers).
3. **Use value types for short-lived objects** (command pools, fences, semaphores).
4. **Do not store raw Vulkan handles** extracted via `handle()` beyond the lifetime of the owning wrapper.
5. **Trust the destruction order.** Dependency `shared_ptr` references ensure correctness automatically.
