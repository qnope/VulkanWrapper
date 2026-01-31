---
sidebar_position: 3
---

# RAII and Ownership

VulkanWrapper uses RAII (Resource Acquisition Is Initialization) to automatically manage Vulkan resource lifetimes. This eliminates manual cleanup and prevents common bugs like leaks and use-after-free.

## Ownership Model

### Shared Ownership (`std::shared_ptr`)

Most Vulkan objects use shared ownership because they're often referenced from multiple places:

```cpp
std::shared_ptr<Instance> instance;
std::shared_ptr<Device> device;
std::shared_ptr<Allocator> allocator;
std::shared_ptr<const Pipeline> pipeline;
std::shared_ptr<const ShaderModule> shader;
```

Resources are automatically freed when the last shared_ptr is destroyed:

```cpp
{
    auto instance = InstanceBuilder().setDebug().build();
    auto device = instance->findGpu().requireGraphicsQueue().find().build();

    // instance and device live until end of scope
}  // Both destroyed here automatically
```

### Unique Handles (`ObjectWithUniqueHandle`)

Some objects wrap Vulkan's unique handles for exclusive ownership:

```cpp
class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    // Pipeline owns the vk::UniquePipeline exclusively
};
```

### Value Types

Lightweight types are often passed by value:

```cpp
struct ColorBlendConfig {
    vk::BlendFactor srcColorBlendFactor;
    // ... other fields
};
```

## Base Classes

### ObjectWithHandle

Base class for objects that wrap a Vulkan handle without ownership:

```cpp
template <typename T>
class ObjectWithHandle {
public:
    [[nodiscard]] T handle() const noexcept;
protected:
    T m_handle;
};
```

### ObjectWithUniqueHandle

Base class for objects with exclusive ownership:

```cpp
template <typename UniqueT>
class ObjectWithUniqueHandle {
public:
    [[nodiscard]] auto handle() const noexcept;
protected:
    UniqueT m_handle;  // Automatically destroyed in destructor
};
```

## Resource Dependencies

VulkanWrapper maintains proper destruction order through shared ownership:

```cpp
class Buffer {
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    VmaAllocation m_allocation;
    // ...
};
```

This ensures:
1. The device stays alive while buffers exist
2. The allocator stays alive while allocations exist
3. Proper cleanup order when everything is destroyed

## Buffer Ownership

Buffers use unique ownership for their internal data:

```cpp
class BufferBase {
    struct Data {
        std::shared_ptr<const Device> m_device;
        std::shared_ptr<const Allocator> m_allocator;
        VmaAllocation m_allocation;
        VkDeviceSize m_size_in_bytes;
    };
    std::unique_ptr<Data> m_data;  // Move-only
};
```

This design:
- Allows buffers to be moved but not copied
- Ensures single ownership of the allocation
- Keeps the Device and Allocator alive

## Typed Buffers

The `Buffer` template provides type-safe buffer operations:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
    // Type information available at compile time
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);
};
```

Usage patterns:

```cpp
// Define buffer types
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;
using StagingBuffer = Buffer<Vertex, true, TransferSrcUsage>;

// Type-safe operations
auto staging = allocator->allocate<StagingBuffer>(count);
staging->write(vertices, 0);  // Only available for HostVisible buffers
```

## Image Ownership

Images follow similar patterns:

```cpp
class Image {
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const Allocator> m_allocator;
    vk::Image m_image;
    VmaAllocation m_allocation;
    // ...
};
```

## Command Buffer Lifetime

Command buffer recording uses RAII through `CommandBufferRecorder`:

```cpp
{
    CommandBufferRecorder recorder(commandBuffer);
    // Recording starts in constructor

    recorder.bindPipeline(pipeline);
    recorder.draw(vertexCount);

}  // Recording ends in destructor (calls vkEndCommandBuffer)
```

## Exception Safety

RAII ensures resources are freed even when exceptions occur:

```cpp
void render() {
    auto buffer = allocator->allocate<VertexBuffer>(1000);

    if (someCondition) {
        throw std::runtime_error("Something failed");
    }

    // Even if exception is thrown, buffer is properly freed
}
```

## Best Practices

1. **Use shared_ptr for long-lived objects** that may be referenced from multiple places
2. **Let builders return smart pointers** - don't manually manage lifetimes
3. **Pass by const reference or shared_ptr** depending on ownership needs
4. **Don't store raw pointers** to resources
5. **Trust the RAII** - avoid manual cleanup code
