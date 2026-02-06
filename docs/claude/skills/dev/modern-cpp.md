# Modern C++23

This project requires C++23. Use modern idioms consistently.

## Concepts (not SFINAE)

```cpp
// Define concepts for type constraints
template <typename T>
concept Vertex = std::is_standard_layout_v<T> && requires(T x) {
    { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
};

// Conditional member functions via requires
void write(std::span<const T> data) requires(HostVisible);

// Constrained templates
template <Vertex V>
auto create_mesh(std::span<const V> vertices);
```

**DO NOT** use `std::enable_if_t` or SFINAE -- always use concepts and `requires` clauses.

## Ranges (not raw loops)

```cpp
// Transform with views
auto positions = std::span(mesh->mVertices, mesh->mNumVertices)
    | std::views::transform([](auto x) { return glm::vec3{x.x, x.y, x.z}; });

// Algorithms
std::ranges::find_if(devices, hasFeature);
std::ranges::sort(items, comparator);
std::erase_if(items, predicate);

// Custom adaptors (Utils/Algos.h)
auto vec = myRange | to<std::vector>;                    // Range -> container
auto handles = objects | vw::to_handle | to<std::vector>; // Extract handles from ObjectWithHandle
```

**DO NOT** write raw `for` loops when ranges/algorithms express the intent more clearly.

## Other C++23 Features Used

```cpp
// consteval (compile-time only evaluation)
static consteval bool does_support(vk::BufferUsageFlags usage);

// Default comparisons (spaceship operator)
auto operator<=>(const Interval&) const noexcept = default;

// source_location for error context (no macros needed)
Exception(std::string msg, std::source_location loc = std::source_location::current());

// Structured bindings
auto [buffer, offset] = bufferList.create_buffer(size);

// std::span for non-owning views
void write(std::span<const T> data, std::size_t offset);

// Designated initializers
vk::RenderingAttachmentInfo colorAttachment{
    .imageView = view->handle(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear
};
```

## Strong Types

The project uses enum-based strong types for type safety:
```cpp
Width{1920}    // not uint32_t
Height{1080}   // not uint32_t
MipLevel{0}    // not uint32_t
Depth{1}       // not uint32_t
```
Always use these instead of raw integers for image/buffer dimensions.
