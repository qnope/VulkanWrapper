# Modern C++23 Patterns

## Concepts (NOT SFINAE)

```cpp
// GOOD: Concept with requires clause
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> &&
    requires(T x) {
        { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
        { T::attribute_descriptions(0, 0)[0] } -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };

// GOOD: Conditional member function with requires
void write(std::span<const T> data, std::size_t offset)
    requires(HostVisible)
{
    BufferBase::write_bytes(data.data(), data.size_bytes(), offset * sizeof(T));
}

// BAD: SFINAE with enable_if
template <typename T, typename = std::enable_if_t<std::is_trivial_v<T>>>
void write(...); // DON'T DO THIS
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Descriptors/Vertex.h`, `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`

---

## Ranges (NOT Raw Loops)

### Views Pipeline

```cpp
// GOOD: Views pipeline
auto positions = std::span(mesh->mVertices, mesh->mNumVertices)
    | std::views::transform([](auto x) { return glm::vec3{x.x, x.y, x.z}; });

auto vertices = std::views::zip(positions, normals, uvs)
    | construct<FullVertex3D>
    | to<std::vector>;

// BAD: Raw loops
std::vector<glm::vec3> positions;
for (size_t i = 0; i < mesh->mNumVertices; ++i) {
    positions.push_back(glm::vec3{...}); // DON'T DO THIS
}
```

### Range Algorithms

```cpp
// Prefer range algorithms
std::ranges::find_if(devices, hasFeature);
std::ranges::none_of(queues, isAvailable);
std::erase_if(m_devices, doesNotSupport);
std::ranges::copy(src, dst.begin());
std::ranges::sort(items, std::less{});
std::ranges::replace(path, '\\', '/');
```

**Reference:** `VulkanWrapper/src/VulkanWrapper/Model/Internal/MeshInfo.cpp`, `VulkanWrapper/src/VulkanWrapper/Vulkan/DeviceFinder.cpp`

---

## Custom Range Adaptors

The project provides custom adaptors in `Algos.h`:

```cpp
#include "VulkanWrapper/Utils/Algos.h"

// Convert range to container
auto vec = myRange | to<std::vector>;
auto set = myRange | to<std::set>;

// Transform and construct objects
auto objects = data | construct<MyType> | to<std::vector>;

// Extract handles from objects
auto handles = objects | to_handle | to<std::vector>;
```

### Implementation Pattern

```cpp
// Generic container conversion
template <template <typename... Ts> typename Container> struct to_t {};

template <typename Range, template <typename... Ts> typename Container>
auto operator|(Range &&range, to_t<Container> /*unused*/) {
    return Container(std::forward<Range>(range).begin(),
                     std::forward<Range>(range).end());
}

// Usage: myRange | to<std::vector>
template <template <typename... Ts> typename Container>
inline constexpr to_t<Container> to;
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Utils/Algos.h`

---

## Compile-Time Computation

### consteval (Compile-Time Only)

```cpp
// GOOD: consteval forces compile-time evaluation
static consteval bool does_support(vk::BufferUsageFlags usage) {
    return (vk::BufferUsageFlags(flags) & usage) == usage;
}
```

### constexpr with Lambdas

```cpp
// GOOD: constexpr with immediate lambda for complex initialization
[[nodiscard]] static constexpr auto attribute_descriptions(int binding, int location) {
    constexpr auto offsets = []() {
        auto sizes = std::array{sizeof(Ts)...};
        std::exclusive_scan(begin(sizes), end(sizes), begin(sizes), 0);
        return sizes;
    }();
    return std::array{
        vk::VertexInputAttributeDescription{
            static_cast<uint32_t>(location),
            static_cast<uint32_t>(binding),
            format_of<Ts>,
            static_cast<uint32_t>(offsets[Is])
        }...
    };
}
```

**Reference:** `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`

---

## std::variant with std::visit

```cpp
using ResourceState = std::variant<ImageState, BufferState, AccelerationStructureState>;

void process(const ResourceState &state) {
    std::visit(
        [this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ImageState>) {
                handle_image(arg);
            } else if constexpr (std::is_same_v<T, BufferState>) {
                handle_buffer(arg);
            } else if constexpr (std::is_same_v<T, AccelerationStructureState>) {
                handle_as(arg);
            }
        },
        state);
}
```

**Reference:** `VulkanWrapper/src/VulkanWrapper/Synchronization/ResourceTracker.cpp`

---

## Other C++23 Features

### std::span for Safe Array Views

```cpp
void write(std::span<const T> data, std::size_t offset);

// Usage
std::vector<Vertex> vertices = ...;
buffer.write(vertices, 0);  // Implicit conversion to span
```

### Default Comparison Operators

```cpp
bool operator==(const InstanceId &other) const noexcept = default;
auto operator<=>(const Interval &other) const noexcept = default;
```

### std::source_location for Error Reporting

```cpp
class Exception : public std::exception {
public:
    explicit Exception(
        std::string message,
        std::source_location location = std::source_location::current());
};
```

### Structured Bindings

```cpp
auto [buffer, offset] = bufferList.create_buffer(size, alignment);
auto [result, value] = device.createBuffer(info);
```
