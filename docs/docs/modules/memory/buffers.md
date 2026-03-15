---
sidebar_position: 2
title: "Buffers"
---

# Buffers

VulkanWrapper provides a type-safe buffer system with compile-time validation of usage flags and host visibility. The template-based `Buffer<T, HostVisible, Flags>` class prevents entire categories of bugs at compile time: you cannot write to a GPU-only buffer, you cannot use a vertex buffer as an index buffer, and buffer sizes are tracked in elements rather than bytes.

## Design Rationale

In raw Vulkan, all buffers are untyped byte ranges. You must manually track the element type, compute byte offsets, check usage flags at runtime, and ensure you only map host-visible buffers. Mistakes are silent -- writing to an unmapped buffer or using the wrong usage flag results in undefined behavior or hard-to-diagnose validation errors.

VulkanWrapper encodes all this information in the C++ type system:

- **`T`** tracks the element type, so `write()` and `read_as_vector()` work in elements, not bytes.
- **`HostVisible`** determines whether `write()` and `read_as_vector()` are available -- trying to call them on a device-local buffer is a **compile error**.
- **`Flags`** encodes the usage flags, enabling compile-time `does_support()` checks and `static_assert` validation in transfer operations.

## API Reference

### BufferBase

```cpp
namespace vw {

class BufferBase : public ObjectWithHandle<vk::Buffer> {
public:
    [[nodiscard]] VkDeviceSize size_bytes() const noexcept;
    [[nodiscard]] vk::DeviceAddress device_address() const;

    void write_bytes(const void *data, VkDeviceSize size, VkDeviceSize offset);
    [[nodiscard]] std::vector<std::byte> read_bytes(VkDeviceSize offset,
                                                    VkDeviceSize size) const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/Buffer.h`

The untyped base class for all buffers. You rarely interact with it directly -- use the typed `Buffer<>` template instead.

#### `size_bytes()`

Returns the total buffer size in bytes.

#### `device_address()`

Returns the buffer's GPU device address (`vk::DeviceAddress`). This is used for buffer device address features -- passing buffer pointers to shaders via push constants or other buffers. All VulkanWrapper usage constants include `eShaderDeviceAddress`, so this is always available.

#### `write_bytes()` / `read_bytes()`

Low-level byte-oriented read/write. The typed `Buffer<>` template provides safer, element-oriented versions.

### Buffer\<T, HostVisible, Flags\>

```cpp
namespace vw {

template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);

    static consteval bool does_support(vk::BufferUsageFlags usage);

    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);

    void write(const T &element, std::size_t offset)
        requires(HostVisible);

    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] std::vector<T> read_as_vector(std::size_t offset,
                                                 std::size_t count) const
        requires(HostVisible);
};

} // namespace vw
```

#### Template Parameters

| Parameter | Description |
|-----------|-------------|
| `T` | Element type stored in the buffer |
| `HostVisible` | `true` for CPU-accessible memory, `false` for GPU-only |
| `Flags` | Buffer usage flags as a `VkBufferUsageFlags` constant |

#### `does_support(vk::BufferUsageFlags usage)` (consteval)

Compile-time check for whether the buffer's flags include the given usage. This is a `consteval` function, so it runs entirely at compile time:

```cpp
using VB = vw::Buffer<Vertex, false, vw::VertexBufferUsage>;
static_assert(VB::does_support(vk::BufferUsageFlagBits::eVertexBuffer));
static_assert(VB::does_support(vk::BufferUsageFlagBits::eTransferDst));
```

#### `write(std::span<const T> data, std::size_t offset)` (requires HostVisible)

Writes a span of elements into the buffer at the given element offset. Only available when `HostVisible` is `true` -- attempting to call this on a device-local buffer is a compile error.

```cpp
std::vector<Vertex> vertices = /* ... */;
buffer.write(vertices, 0);           // Write all vertices starting at element 0
buffer.write(vertices, 100);         // Write starting at element 100
```

#### `write(const T &element, std::size_t offset)` (requires HostVisible)

Writes a single element at the given element offset:

```cpp
buffer.write(myVertex, 42);  // Write one vertex at position 42
```

#### `size()`

Returns the number of elements (not bytes) the buffer can hold:

```cpp
auto buf = vw::create_buffer<float, true, vw::UniformBufferUsage>(
    *allocator, 100);
buf.size();        // 100
buf.size_bytes();  // 400 (100 * sizeof(float))
```

#### `read_as_vector(std::size_t offset, std::size_t count)` (requires HostVisible)

Reads elements from the buffer back to the CPU, returning them as a `std::vector<T>`. Only available for host-visible buffers.

```cpp
auto data = buffer.read_as_vector(0, buffer.size());
```

### Usage Constants

```cpp
namespace vw {

constexpr VkBufferUsageFlags2 VertexBufferUsage = /* ... */;
constexpr VkBufferUsageFlags2 IndexBufferUsage = /* ... */;
constexpr VkBufferUsageFlags2 UniformBufferUsage = /* ... */;
constexpr VkBufferUsageFlags2 StorageBufferUsage = /* ... */;
constexpr VkBufferUsageFlags2 StagingBufferUsage = /* ... */;

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/BufferUsage.h`

Each constant is a combination of the primary usage flag plus common auxiliary flags needed for typical workflows:

| Constant | Primary Usage | Also Includes |
|----------|--------------|---------------|
| `VertexBufferUsage` | `eVertexBuffer` | `eTransferDst`, `eShaderDeviceAddress`, `eAccelerationStructureBuildInputReadOnlyKHR` |
| `IndexBufferUsage` | `eIndexBuffer` | `eTransferDst`, `eShaderDeviceAddress`, `eAccelerationStructureBuildInputReadOnlyKHR` |
| `UniformBufferUsage` | `eUniformBuffer` | `eTransferDst`, `eShaderDeviceAddress` |
| `StorageBufferUsage` | `eStorageBuffer` | `eTransferDst`, `eShaderDeviceAddress` |
| `StagingBufferUsage` | `eTransferSrc` | `eTransferDst`, `eShaderDeviceAddress` |

The `eTransferDst` flag on vertex/index/storage buffers enables uploading data via staging buffers. The `eShaderDeviceAddress` flag enables buffer device address access from shaders. The `eAccelerationStructureBuildInputReadOnlyKHR` flag on vertex and index buffers allows their use as input for ray tracing acceleration structure builds.

### Type Alias: IndexBuffer

```cpp
namespace vw {
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;
} // namespace vw
```

Defined in `fwd.h`. A common type alias for index buffers storing 32-bit unsigned integers in device-local memory.

### AllocateBufferUtils

```cpp
namespace vw {

template <typename T, bool HostVisible>
Buffer<T, HostVisible, VertexBufferUsage>
allocate_vertex_buffer(const Allocator &allocator, vk::DeviceSize size);

template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
Buffer<T, HostVisible, Usage>
create_buffer(const Allocator &allocator, vk::DeviceSize number_elements);

template <typename BufferType>
BufferType create_buffer(const Allocator &allocator,
                         vk::DeviceSize number_elements);

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/AllocateBufferUtils.h`

These are the primary factory functions for creating typed buffers. They compute byte sizes from element counts and forward to `Allocator::allocate_buffer()` internally.

#### `allocate_vertex_buffer<T, HostVisible>(allocator, count)`

Creates a buffer with `VertexBufferUsage` flags. The template parameter `T` is the vertex type and `count` is the number of vertices:

```cpp
auto vbuf = vw::allocate_vertex_buffer<Vertex, false>(*allocator, 1000);
// Type: Buffer<Vertex, false, VertexBufferUsage>
```

#### `create_buffer<T, HostVisible, Usage>(allocator, count)`

Creates a buffer with any usage constant. This is the most flexible factory:

```cpp
auto ubuf = vw::create_buffer<UniformData, true, vw::UniformBufferUsage>(
    *allocator, 1);

auto sbuf = vw::create_buffer<float, false, vw::StorageBufferUsage>(
    *allocator, 4096);

auto staging = vw::create_buffer<std::byte, true, vw::StagingBufferUsage>(
    *allocator, 65536);
```

#### `create_buffer<BufferType>(allocator, count)`

An alternative overload that deduces `T`, `HostVisible`, and `Usage` from a buffer type alias:

```cpp
using MyBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
auto buf = vw::create_buffer<MyBuffer>(*allocator, 100);
```

### UniformBufferAllocator

```cpp
namespace vw {

template <typename T> struct UniformBufferChunk {
    vk::Buffer handle;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    uint32_t index;

    vk::DescriptorBufferInfo descriptorInfo() const;
    void write(const T &value);
    void write(std::span<const T> data);
};

class UniformBufferAllocator {
public:
    UniformBufferAllocator(std::shared_ptr<const Allocator> allocator,
                           vk::DeviceSize totalSize,
                           vk::DeviceSize minAlignment = 256);

    template <typename T>
    std::optional<UniformBufferChunk<T>> allocate(size_t count = 1);

    void deallocate(uint32_t index);
    void clear();

    [[nodiscard]] vk::Buffer buffer() const;
    [[nodiscard]] vk::DeviceSize totalSize() const;
    [[nodiscard]] vk::DeviceSize freeSpace() const;
    [[nodiscard]] size_t allocationCount() const;
};

} // namespace vw
```

**Header:** `VulkanWrapper/Memory/UniformBufferAllocator.h`

A sub-allocator that carves chunks out of a single large uniform buffer. This is useful for dynamic uniform buffer descriptors where many small uniform blocks share a single buffer with different offsets.

Key features:

- **Alignment-aware:** Each allocation is aligned to `minAlignment` (default 256 bytes, matching typical GPU requirements).
- **Write-through:** `UniformBufferChunk::write()` writes directly to the backing buffer at the chunk's offset.
- **Descriptor-ready:** `UniformBufferChunk::descriptorInfo()` returns a `vk::DescriptorBufferInfo` ready for descriptor set updates.

```cpp
vw::UniformBufferAllocator uboAllocator(allocator, 65536);

auto chunk = uboAllocator.allocate<MyUniformData>();
if (chunk) {
    chunk->write(myData);
    auto descInfo = chunk->descriptorInfo();
    // Use descInfo in descriptor set update...
}
```

## Usage Patterns

### Defining Buffer Type Aliases

A common pattern is to define type aliases for your buffer types at the top of a file or in a header:

```cpp
using VertexBuffer = vw::Buffer<Vertex, false, vw::VertexBufferUsage>;
using StagingVertexBuffer = vw::Buffer<Vertex, true, vw::StagingBufferUsage>;
using UniformBuffer = vw::Buffer<UniformData, true, vw::UniformBufferUsage>;
```

This makes the code self-documenting: the type name tells you exactly what the buffer is for.

### CPU-to-GPU Upload Pattern

Device-local buffers cannot be written from the CPU. The standard pattern is:

1. Create a host-visible staging buffer.
2. Write data into the staging buffer.
3. Issue a GPU copy command from staging to the device-local buffer.
4. Wait for the copy to complete before using the buffer.

```cpp
// Device-local vertex buffer
auto gpuBuffer = vw::allocate_vertex_buffer<Vertex, false>(*allocator, count);

// Host-visible staging buffer
auto staging = vw::create_buffer<Vertex, true, vw::StagingBufferUsage>(
    *allocator, count);

// Write to staging
staging.write(vertices, 0);

// Copy on GPU (via command buffer)
vk::BufferCopy region{
    .srcOffset = 0,
    .dstOffset = 0,
    .size = staging.size_bytes()
};
cmd.copyBuffer(staging.handle(), gpuBuffer.handle(), region);
```

For batched uploads, use `StagingBufferManager` -- see [Staging & Transfers](staging.md).

### Compile-Time Usage Validation

You can verify buffer capabilities at compile time:

```cpp
using VB = vw::Buffer<Vertex, false, vw::VertexBufferUsage>;

// These are compile-time checks:
static_assert(VB::does_support(vk::BufferUsageFlagBits::eVertexBuffer));
static_assert(VB::does_support(vk::BufferUsageFlagBits::eTransferDst));
static_assert(!VB::host_visible);  // GPU-only
```

The `StagingBufferManager::fill_buffer()` template uses `static_assert` internally to verify that the destination buffer has `eTransferDst` set, catching misuse at compile time.

### Reading Back Data from the GPU

For screenshots, debugging, or compute results:

```cpp
auto readback = vw::create_buffer<float, true, vw::StagingBufferUsage>(
    *allocator, elementCount);

// Copy GPU buffer to readback buffer (via command buffer)
vk::BufferCopy region{.size = readback.size_bytes()};
cmd.copyBuffer(gpuBuffer.handle(), readback.handle(), region);

// After submitting and waiting...
auto results = readback.read_as_vector(0, elementCount);
```

## Common Pitfalls

### Trying to `write()` to a device-local buffer

**Symptom:** Compile error -- `write()` is constrained with `requires(HostVisible)`.

**Cause:** Device-local buffers (`HostVisible = false`) are not CPU-accessible. You cannot map them or write to them from the CPU.

**Fix:** Use a staging buffer and a GPU copy command to upload data. Or, if the buffer needs frequent CPU updates, make it host-visible:

```cpp
// Device-local (no write):
auto buf = vw::create_buffer<float, false, vw::StorageBufferUsage>(*alloc, n);

// Host-visible (has write):
auto buf = vw::create_buffer<float, true, vw::UniformBufferUsage>(*alloc, n);
buf.write(data, 0);
```

### Confusing element counts and byte sizes

**Symptom:** Writing past the end of a buffer, or allocating a buffer that is too small/large.

**Cause:** `create_buffer<>()` and `allocate_vertex_buffer<>()` take element counts, but `Allocator::allocate_buffer()` and `allocate_index_buffer()` take byte sizes.

**Fix:** Prefer the typed helpers (`create_buffer<>()`, `allocate_vertex_buffer<>()`) which handle the byte calculation for you. If you must use the low-level `allocate_buffer()`, compute bytes explicitly:

```cpp
allocator->allocate_buffer(
    count * sizeof(MyType),  // explicit byte calculation
    host_visible, usage, sharing_mode);
```

### Forgetting that `Buffer` is move-only

**Symptom:** Compile error when trying to copy a buffer.

**Cause:** `BufferBase` (and therefore `Buffer<>`) is move-only. Copying a buffer would be ambiguous -- should it copy the GPU memory too?

**Fix:** Use `std::move()` when transferring ownership:

```cpp
auto buf = vw::create_buffer<float, true, vw::UniformBufferUsage>(*alloc, 100);
auto buf2 = std::move(buf);  // OK
// buf is now in a moved-from state
```

### Using the wrong usage constant

**Symptom:** Validation error about buffer usage flags when binding or copying.

**Cause:** Using, for example, `StorageBufferUsage` for a buffer that you try to bind as a vertex buffer.

**Fix:** Match the usage constant to how you intend to use the buffer. Use `does_support()` at compile time to verify:

```cpp
using MyBuf = vw::Buffer<float, false, vw::StorageBufferUsage>;
static_assert(MyBuf::does_support(vk::BufferUsageFlagBits::eStorageBuffer));
// This would fail:
// static_assert(MyBuf::does_support(vk::BufferUsageFlagBits::eVertexBuffer));
```

## Integration with Other Modules

| What | How | See |
|------|-----|-----|
| Allocate buffers | `create_buffer<>()`, `allocate_vertex_buffer<>()` | This page |
| Batch uploads | `StagingBufferManager::fill_buffer()` | [Staging & Transfers](staging.md) |
| Memory allocation | `Allocator::allocate_buffer()` | [Allocator](allocator.md) |
| Descriptor binding | `buffer.handle()` for `vk::DescriptorBufferInfo` | Descriptors module |
| Ray tracing input | Vertex/index buffers with `eAccelerationStructureBuildInputReadOnlyKHR` | Ray Tracing module |
