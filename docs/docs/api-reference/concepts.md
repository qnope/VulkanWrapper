---
sidebar_position: 3
---

# Concepts

C++ concepts used in VulkanWrapper for type constraints.

## Vertex Concept

Constrains types that can be used as vertex data:

```cpp
template <typename T>
concept Vertex = requires(unsigned binding, unsigned location) {
    // Must provide binding description
    { T::binding_description(binding) }
        -> std::convertible_to<vk::VertexInputBindingDescription>;

    // Must provide attribute descriptions
    { T::attribute_descriptions(binding, location) }
        -> std::ranges::range;
};
```

### Implementing Vertex

```cpp
struct MyVertex {
    glm::vec3 position;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription
    binding_description(unsigned binding) {
        return {
            .binding = binding,
            .stride = sizeof(MyVertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2>
    attribute_descriptions(unsigned binding, unsigned location) {
        return {{
            {location + 0, binding, vk::Format::eR32G32B32Sfloat,
             offsetof(MyVertex, position)},
            {location + 1, binding, vk::Format::eR32G32Sfloat,
             offsetof(MyVertex, texCoord)}
        }};
    }
};

static_assert(Vertex<MyVertex>);
```

### Built-in Vertex Types

```cpp
// All satisfy the Vertex concept
static_assert(Vertex<ColoredVertex<glm::vec2>>);
static_assert(Vertex<ColoredVertex<glm::vec3>>);
static_assert(Vertex<ColoredAndTexturedVertex<glm::vec2>>);
static_assert(Vertex<ColoredAndTexturedVertex<glm::vec3>>);
static_assert(Vertex<FullVertex3D>);
```

### Usage

```cpp
// Pipeline builder uses the concept
template <Vertex V>
GraphicsPipelineBuilder& add_vertex_binding();

// Usage
builder.add_vertex_binding<FullVertex3D>();    // OK
builder.add_vertex_binding<int>();             // Error: int doesn't satisfy Vertex
```

## Buffer Type Constraints

### Host Visibility

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
    // Only available when HostVisible is true
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);

    [[nodiscard]] std::vector<T> read_as_vector(
        std::size_t offset, std::size_t count) const
        requires(HostVisible);
};
```

### Usage Validation

```cpp
// Compile-time check for buffer usage
static consteval bool does_support(vk::BufferUsageFlags usage) {
    return (vk::BufferUsageFlags(Flags) & usage) == usage;
}

// Usage
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;

static_assert(VertexBuffer::does_support(
    vk::BufferUsageFlagBits::eVertexBuffer));  // OK

static_assert(VertexBuffer::does_support(
    vk::BufferUsageFlagBits::eUniformBuffer)); // Fails
```

## Type Traits

### Strong Types

VulkanWrapper uses strong types for dimensions:

```cpp
struct Width {
    uint32_t value;
    constexpr explicit Width(uint32_t v) : value(v) {}
};

struct Height {
    uint32_t value;
    constexpr explicit Height(uint32_t v) : value(v) {}
};

struct Depth {
    uint32_t value;
    constexpr explicit Depth(uint32_t v) : value(v) {}
};

struct MipLevel {
    uint32_t value;
    constexpr explicit MipLevel(uint32_t v) : value(v) {}
};

struct ArrayLayer {
    uint32_t value;
    constexpr explicit ArrayLayer(uint32_t v) : value(v) {}
};
```

### Usage

```cpp
// Strong types prevent mistakes
void resize(Width width, Height height);

resize(Width{1920}, Height{1080});  // OK
resize(Height{1080}, Width{1920});  // Compile error!
resize(1920, 1080);                 // Compile error!
```

## Ranges Support

VulkanWrapper uses C++20 ranges throughout:

```cpp
// Span for safe array views
void write(std::span<const T> data, std::size_t offset);

// Range algorithms
std::ranges::sort(vertices);
auto filtered = vertices | std::views::filter(predicate);

// Range iteration
for (const auto& instance : scene.instances()) {
    // ...
}
```

## Future Concepts

Potential concepts for future versions:

```cpp
// Allocatable concept
template <typename T>
concept Allocatable = requires {
    typename T::type;
    { T::host_visible } -> std::convertible_to<bool>;
    { T::flags } -> std::convertible_to<vk::BufferUsageFlags>;
};

// RenderPass concept
template <typename T>
concept RenderPass = requires(T pass, CommandBuffer& cmd) {
    { pass.render(cmd) } -> std::same_as<ImageView&>;
};
```
