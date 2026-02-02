# Modern C++23

## Concepts (not SFINAE)

```cpp
template <typename T>
concept Vertex = std::is_standard_layout_v<T> && requires(T x) {
    { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
};

// Conditional member
void write(std::span<const T> data) requires(HostVisible);
```

## Ranges (not loops)

```cpp
auto positions = std::span(mesh->mVertices, mesh->mNumVertices)
    | std::views::transform([](auto x) { return glm::vec3{x.x, x.y, x.z}; });

std::ranges::find_if(devices, hasFeature);
std::erase_if(items, predicate);

// Custom adaptors (Algos.h)
auto vec = myRange | to<std::vector>;
auto handles = objects | to_handle | to<std::vector>;
```

## Other Features

```cpp
// consteval (compile-time only)
static consteval bool does_support(vk::BufferUsageFlags usage);

// Default comparisons
auto operator<=>(const Interval&) const noexcept = default;

// source_location for errors
Exception(std::string msg, std::source_location loc = std::source_location::current());

// Structured bindings
auto [buffer, offset] = bufferList.create_buffer(size);
```
