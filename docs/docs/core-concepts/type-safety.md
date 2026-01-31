---
sidebar_position: 4
---

# Type Safety

VulkanWrapper leverages C++23 features to provide compile-time type safety, catching errors before runtime.

## Strong Types

### Dimensional Types

Prevent accidental parameter swapping with strong types:

```cpp
// Strong type definitions
struct Width { uint32_t value; };
struct Height { uint32_t value; };
struct Depth { uint32_t value; };
struct MipLevel { uint32_t value; };

// Usage - can't accidentally swap width and height
void resize(Width width, Height height);

resize(Width{1920}, Height{1080});  // Correct
resize(Height{1080}, Width{1920});  // Compile error!
```

### Buffer Types

Buffer usage is validated at compile time:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);

    // Compile-time usage validation
    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (flags & usage) == usage;
    }

    // Only available for HostVisible buffers
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);
};
```

Usage:

```cpp
// Define typed buffer aliases
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;
using StagingBuffer = Buffer<Vertex, true, TransferSrcUsage>;

// Compile-time validation
static_assert(VertexBuffer::does_support(
    vk::BufferUsageFlagBits::eVertexBuffer));

// Host visibility enforced at compile time
StagingBuffer staging = /* ... */;
staging.write(data, 0);  // OK - StagingBuffer is host visible

VertexBuffer vertex = /* ... */;
vertex.write(data, 0);   // Compile error! Not host visible
```

## Concepts

VulkanWrapper uses C++20/23 concepts to constrain templates:

### Vertex Concept

```cpp
template <typename T>
concept Vertex = requires(unsigned binding, unsigned location) {
    { T::binding_description(binding) }
        -> std::convertible_to<vk::VertexInputBindingDescription>;
    { T::attribute_descriptions(binding, location) }
        -> std::ranges::range;
};
```

Usage:

```cpp
// Pipeline builder validates vertex types
template <Vertex V>
GraphicsPipelineBuilder& add_vertex_binding();

// Usage
builder.add_vertex_binding<FullVertex3D>();  // OK
builder.add_vertex_binding<int>();           // Compile error!
```

### Built-in Vertex Types

VulkanWrapper provides several vertex implementations:

```cpp
// Position + Color
template <typename Position>
struct ColoredVertex {
    Position position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription
    binding_description(unsigned binding);

    static std::array<vk::VertexInputAttributeDescription, 2>
    attribute_descriptions(unsigned binding, unsigned location);
};

// Position + Color + UV
template <typename Position>
struct ColoredAndTexturedVertex {
    Position position;
    glm::vec3 color;
    glm::vec2 uv;
    // ...
};

// Full 3D vertex
struct FullVertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 uv;
    // ...
};
```

## Consteval Validation

Use `consteval` for compile-time-only validation:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(Flags) & usage) == usage;
    }
};

// Checked at compile time
static_assert(IndexBuffer::does_support(
    vk::BufferUsageFlagBits::eIndexBuffer));
```

## Requires Clauses

Conditional member functions based on template parameters:

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
    // Only available when buffer is host visible
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);

    // Only available when buffer is host visible
    [[nodiscard]] std::vector<T> read_as_vector(
        std::size_t offset, std::size_t count) const
        requires(HostVisible);
};
```

## Span for Safe Array Views

Use `std::span` instead of raw pointers:

```cpp
void write(std::span<const T> data, std::size_t offset);

// Usage - size is carried with the data
std::vector<Vertex> vertices = /* ... */;
buffer.write(vertices, 0);  // span created automatically

// Or explicitly
buffer.write(std::span{vertices.data(), 10}, 0);
```

## Format Type Safety

Image formats are validated:

```cpp
auto image = ImageBuilder()
    .setFormat(vk::Format::eR8G8B8A8Srgb)  // Type-safe format
    .setExtent(Width{1024}, Height{1024})  // Strong types
    .build();
```

## Pipeline Attachments

Color attachment configuration is type-safe:

```cpp
struct ColorBlendConfig {
    vk::BlendFactor srcColorBlendFactor;
    vk::BlendFactor dstColorBlendFactor;
    // ... all fields use vk:: enums

    static ColorBlendConfig alpha();
    static ColorBlendConfig additive();
    static ColorBlendConfig constant_blend();
};

// Usage
builder.add_color_attachment(
    vk::Format::eR8G8B8A8Srgb,
    ColorBlendConfig::alpha()
);
```

## Best Practices

1. **Use strong types** for dimensions and indices
2. **Define buffer type aliases** with clear names
3. **Use concepts** to constrain template parameters
4. **Prefer `std::span`** over raw pointers
5. **Use `consteval`** for compile-time validation
6. **Add `requires` clauses** for conditional functionality
