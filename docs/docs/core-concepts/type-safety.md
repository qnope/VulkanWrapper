---
sidebar_position: 4
---

# Type Safety

VulkanWrapper uses C++23 features to catch mistakes at compile time rather than at runtime. This page covers the four main type safety mechanisms: strong types, template buffer validation, `consteval` compile-time checks, and concepts with `requires` clauses.

## Strong Types

### Preventing Parameter Mixups

Vulkan functions often take multiple `uint32_t` parameters in a row. It is easy to accidentally swap width and height, or pass a mip level where a layer count is expected. VulkanWrapper wraps these in distinct types:

```cpp
Width{1920}     // image width
Height{1080}    // image height
Depth{1}        // image depth
MipLevel{0}     // mipmap level
```

Because each is a separate type, the compiler rejects mixups:

```cpp
// Correct -- types match the parameter expectations
allocator->create_image_2D(Width{1920}, Height{1080}, false, format, usage);

// Compile error -- Width and Height are swapped
allocator->create_image_2D(Height{1080}, Width{1920}, false, format, usage);
```

This catches a common class of bugs that would otherwise silently produce wrong results.

## Type-Safe Buffers

### The Buffer Template

VulkanWrapper's `Buffer` is a class template with three parameters:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer;
```

- **`T`** -- the element type stored in the buffer (e.g., `float`, `Vertex`).
- **`HostVisible`** -- whether the CPU can read/write the buffer directly.
- **`Flags`** -- the Vulkan buffer usage flags (vertex, index, uniform, etc.).

All three are encoded in the type itself, so the compiler can enforce constraints that raw Vulkan cannot.

### Usage Constants

VulkanWrapper provides named constants for common buffer usages (defined in `BufferUsage.h`):

| Constant | Purpose |
|----------|---------|
| `VertexBufferUsage` | Vertex buffer |
| `IndexBufferUsage` | Index buffer |
| `UniformBufferUsage` | Uniform buffer |
| `StorageBufferUsage` | Storage buffer |
| `StagingBufferUsage` | CPU-to-GPU staging buffer |

### Creating Buffers

Use the factory functions instead of constructing buffers directly:

```cpp
// General factory: create_buffer<ElementType, HostVisible, UsageConstant>
auto uniformBuf = create_buffer<float, true, UniformBufferUsage>(*allocator, 100);

// Convenience factory for vertex buffers:
// allocate_vertex_buffer<VertexType, HostVisible>(allocator, count)
auto vertexBuf = allocate_vertex_buffer<FullVertex3D, false>(*allocator, 1000);
```

The template parameters are part of the type, so you cannot accidentally use a vertex buffer where an index buffer is expected.

## Compile-Time Validation with consteval

The `Buffer` class provides a `consteval` method to check usage support at compile time:

```cpp
static consteval bool does_support(vk::BufferUsageFlags usage);
```

Because `does_support` is `consteval`, it can only be called in constant expressions. This means usage validation happens entirely during compilation:

```cpp
using MyVertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;

// Checked at compile time -- passes
static_assert(MyVertexBuffer::does_support(
    vk::BufferUsageFlagBits::eVertexBuffer));

// Checked at compile time -- fails if VertexBufferUsage doesn't include transfer
static_assert(MyVertexBuffer::does_support(
    vk::BufferUsageFlagBits::eTransferDst));
```

If the assertion fails, you get a clear compiler error instead of a runtime Vulkan validation error.

## Concepts

### The Vertex Concept

VulkanWrapper defines a `Vertex` concept that constrains what types can be used as vertex data in pipeline builders:

```cpp
template <typename T>
concept Vertex = requires(unsigned binding, unsigned location) {
    { T::binding_description(binding) }
        -> std::convertible_to<vk::VertexInputBindingDescription>;
    { T::attribute_descriptions(binding, location) }
        -> std::ranges::range;
};
```

A type satisfies `Vertex` if it provides two static methods:
- `binding_description(binding)` returning a binding description.
- `attribute_descriptions(binding, location)` returning a range of attribute descriptions.

The pipeline builder uses this concept to reject non-vertex types at compile time:

```cpp
// OK -- FullVertex3D satisfies the Vertex concept
builder.add_vertex_binding<FullVertex3D>();

// Compile error -- int does not satisfy the Vertex concept
builder.add_vertex_binding<int>();
```

### Built-In Vertex Types

VulkanWrapper provides several vertex types that satisfy the `Vertex` concept:

```cpp
// Position + Color (templated on position type)
ColoredVertex<glm::vec3>

// Position + Color + UV (templated on position type)
ColoredAndTexturedVertex<glm::vec3>

// Full 3D vertex with position, normal, tangent, bitangent, and UV
FullVertex3D
```

You can also define your own vertex types -- just implement the two required static methods.

## requires Clauses for Conditional Methods

Some buffer operations only make sense under certain conditions. VulkanWrapper uses `requires` clauses to make these constraints explicit:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
    // Only available when the buffer is host-visible
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);

    // Only available when the buffer is host-visible
    [[nodiscard]] std::vector<T> read_as_vector(
        std::size_t offset, std::size_t count) const
        requires(HostVisible);
};
```

If you try to call `write()` on a device-local (non-host-visible) buffer, the compiler produces a clear error:

```cpp
auto gpuBuffer = create_buffer<float, false, VertexBufferUsage>(*allocator, 100);
gpuBuffer.write(data, 0);  // Compile error: requires(HostVisible) not satisfied

auto cpuBuffer = create_buffer<float, true, StagingBufferUsage>(*allocator, 100);
cpuBuffer.write(data, 0);  // OK: HostVisible is true
```

This is far better than a runtime crash or a validation layer warning -- the mistake is caught before the program even runs.

## std::span for Safe Array Views

VulkanWrapper uses `std::span` instead of raw pointer-plus-size pairs:

```cpp
void write(std::span<const T> data, std::size_t offset);
```

`std::span` carries the data pointer and size together, preventing buffer overruns. It also converts implicitly from `std::vector`, `std::array`, and C arrays:

```cpp
std::vector<float> values = {1.0f, 2.0f, 3.0f};

// std::span created implicitly from the vector
buffer.write(values, 0);

// Explicit span with a subset of elements
buffer.write(std::span{values.data(), 2}, 0);
```

## Pipeline Attachment Type Safety

Color attachment configuration uses `vk::` enum types and static factory methods:

```cpp
builder.add_color_attachment(
    vk::Format::eR8G8B8A8Srgb,        // type-safe format enum
    ColorBlendConfig::alpha()           // named preset
);
```

`ColorBlendConfig` provides static methods for common blending modes (`alpha()`, `additive()`, etc.), avoiding the need to manually specify six separate blend factor and blend op parameters.

## Summary

| Mechanism | What It Catches | When |
|-----------|----------------|------|
| Strong types (`Width`, `Height`, ...) | Swapped parameters | Compile time |
| `Buffer<T, HostVisible, Flags>` | Wrong buffer usage | Compile time |
| `consteval does_support()` | Unsupported buffer operations | Compile time |
| `Vertex` concept | Non-vertex types in pipeline | Compile time |
| `requires(HostVisible)` | CPU access to device-local buffers | Compile time |
| `std::span` | Buffer overruns | Compile time (size) + runtime (bounds) |
