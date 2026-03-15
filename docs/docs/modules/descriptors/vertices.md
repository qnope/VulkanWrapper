---
sidebar_position: 3
---

# Vertices

VulkanWrapper provides a type-safe vertex system built on C++23 concepts and a
compile-time attribute layout helper called `VertexInterface`. This page
explains the built-in vertex types, shows how to create your own, and
demonstrates integration with `GraphicsPipelineBuilder`.

## The Vertex Concept

Any type that satisfies the `vw::Vertex` concept can be used with
`GraphicsPipelineBuilder::add_vertex_binding<V>()`. The concept is defined in
`VulkanWrapper/Descriptors/Vertex.h`:

```cpp
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> &&
    requires(T x) {
        {
            T::binding_description(0)
        } -> std::same_as<vk::VertexInputBindingDescription>;
        {
            T::attribute_descriptions(0, 0)[0]
        } -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };
```

A vertex type must:

1. Be **standard-layout** and **trivially copyable** (so it can be
   `memcpy`-ed to GPU buffers).
2. Provide a **static** `binding_description(int binding)` that returns a
   `vk::VertexInputBindingDescription`.
3. Provide a **static** `attribute_descriptions(int binding, int location)`
   that returns a container whose elements are convertible to
   `vk::VertexInputAttributeDescription`.

## VertexInterface -- The Easy Path

Rather than implementing those two static functions by hand, you can inherit
from `VertexInterface<Ts...>`. It automatically computes the binding stride
(the sum of `sizeof(Ts)...`) and generates attribute descriptions with
correct offsets and formats using `format_from<T>`:

```cpp
template <typename... Ts> class VertexInterface {
  public:
    static constexpr auto binding_description(int binding);
    static constexpr auto attribute_descriptions(int binding, int location);
};
```

The supported format mappings are:

| C++ Type    | Vulkan Format              |
|-------------|----------------------------|
| `float`     | `vk::Format::eR32Sfloat`          |
| `glm::vec2` | `vk::Format::eR32G32Sfloat`       |
| `glm::vec3` | `vk::Format::eR32G32B32Sfloat`    |
| `glm::vec4` | `vk::Format::eR32G32B32A32Sfloat` |

## Built-in Vertex Types

### Vertex3D

The simplest 3D vertex -- position only:

```cpp
struct Vertex3D : VertexInterface<glm::vec3> {
    glm::vec3 position;
};
```

| Location | Type      | Field      |
|----------|-----------|------------|
| 0        | `vec3`    | `position` |

### ColoredVertex / ColoredVertex3D

Position with an RGB color. The template parameter selects 2D or 3D position:

```cpp
template <typename Position>
struct ColoredVertex : VertexInterface<Position, glm::vec3> {
    Position  position{};
    glm::vec3 color{};
};

using ColoredVertex2D = ColoredVertex<glm::vec2>;
using ColoredVertex3D = ColoredVertex<glm::vec3>;
```

| Location | Type      | Field      |
|----------|-----------|------------|
| 0        | `vec2`/`vec3` | `position` |
| 1        | `vec3`    | `color`    |

### ColoredAndTexturedVertex

Position, color, and texture coordinates:

```cpp
template <typename Position>
struct ColoredAndTexturedVertex
    : VertexInterface<Position, glm::vec3, glm::vec2> {
    Position  position{};
    glm::vec3 color{};
    glm::vec2 texCoord{};
};

using ColoredAndTexturedVertex2D = ColoredAndTexturedVertex<glm::vec2>;
using ColoredAndTexturedVertex3D = ColoredAndTexturedVertex<glm::vec3>;
```

### FullVertex3D

Complete vertex for PBR rendering -- position, normal, tangent, bitangent,
and UV:

```cpp
struct FullVertex3D
    : VertexInterface<glm::vec3, glm::vec3, glm::vec3, glm::vec3, glm::vec2> {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 tangeant{};
    glm::vec3 bitangeant{};
    glm::vec2 uv{};
};
```

| Location | Type      | Field        |
|----------|-----------|--------------|
| 0        | `vec3`    | `position`   |
| 1        | `vec3`    | `normal`     |
| 2        | `vec3`    | `tangeant`   |
| 3        | `vec3`    | `bitangeant` |
| 4        | `vec2`    | `uv`         |

:::note
The field names `tangeant` and `bitangeant` use the French spelling found in the
source code, not the English "tangent" / "bitangent".
:::

## Creating Custom Vertex Types

### Using VertexInterface (recommended)

The simplest approach -- list your member types as template arguments:

```cpp
struct MyVertex : vw::VertexInterface<glm::vec3, glm::vec2, glm::vec4> {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec4 color;
};
```

`VertexInterface` derives the offsets via `std::exclusive_scan` over the sizes.
The member order **must** match the template argument order.

### Manual Implementation

If you need non-standard formats or per-instance input rate, implement the
two static functions yourself:

```cpp
struct InstanceData {
    glm::mat4 model; // occupies 4 vec4 locations

    static vk::VertexInputBindingDescription
    binding_description(int binding) {
        return vk::VertexInputBindingDescription(
            binding, sizeof(InstanceData),
            vk::VertexInputRate::eInstance);
    }

    static std::array<vk::VertexInputAttributeDescription, 4>
    attribute_descriptions(int binding, int location) {
        std::array<vk::VertexInputAttributeDescription, 4> attrs;
        for (int i = 0; i < 4; ++i) {
            attrs[i].binding  = binding;
            attrs[i].location = location + i;
            attrs[i].format   = vk::Format::eR32G32B32A32Sfloat;
            attrs[i].offset   = sizeof(glm::vec4) * i;
        }
        return attrs;
    }
};
```

## Integration with GraphicsPipelineBuilder

`GraphicsPipelineBuilder::add_vertex_binding<V>()` is a template method
constrained by the `Vertex` concept. It automatically assigns binding and
location indices in the order you call it:

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_vertex_binding<vw::FullVertex3D>()     // binding 0, locations 0-4
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertModule)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragModule)
    .add_color_attachment(vk::Format::eR8G8B8A8Srgb)
    .with_dynamic_viewport_scissor()
    .build();
```

### Multiple Bindings

You can call `add_vertex_binding` multiple times for split vertex streams.
Bindings and locations are assigned sequentially:

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_vertex_binding<vw::Vertex3D>()         // binding 0, location 0
    .add_vertex_binding<InstanceData>()          // binding 1, locations 1-4
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertModule)
    .add_color_attachment(swapchainFormat)
    .with_dynamic_viewport_scissor()
    .build();
```

When drawing, bind both buffers:

```cpp
cmd.bindVertexBuffers(0, {positionBuffer, instanceBuffer}, {0, 0});
```

## Matching Shader Layouts

Your GLSL vertex shader must declare `in` variables at the locations that
match your vertex type. For `FullVertex3D`:

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

void main() {
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
```

## Creating Vertex Buffers

Use `allocate_vertex_buffer` to create a typed GPU buffer for your vertex
type:

```cpp
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>

// Create a device-local vertex buffer for 3 vertices
auto vertexBuffer =
    vw::allocate_vertex_buffer<vw::FullVertex3D, false>(*allocator, 3);
```

Upload data with `StagingBufferManager`:

```cpp
std::vector<vw::FullVertex3D> vertices = { /* ... */ };
stagingBufferManager.fill_buffer(
    std::span<const vw::FullVertex3D>{vertices},
    vertexBuffer, 0);
```

## Best Practices

1. **Prefer `VertexInterface`** over manual implementations -- it eliminates
   offset calculation errors.
2. **Keep member order** identical to the template argument order in
   `VertexInterface`.
3. **Align attributes** to 4-byte boundaries for optimal GPU performance.
4. **Use `FullVertex3D`** for meshes loaded via the model importer -- it
   matches the importer's output format.
5. **Match shader locations** exactly to the attribute order generated by
   your vertex type.
