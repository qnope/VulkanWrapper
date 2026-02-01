---
sidebar_position: 3
---

# Vertices

VulkanWrapper provides a type-safe vertex system using C++ concepts.

## Overview

```cpp
#include <VulkanWrapper/Descriptors/Vertex.h>

// Use built-in vertex type
using Vertex = vw::FullVertex3D;

// Or define custom vertex
struct MyVertex {
    glm::vec3 position;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription
    binding_description(unsigned binding);

    static std::array<vk::VertexInputAttributeDescription, 2>
    attribute_descriptions(unsigned binding, unsigned location);
};
```

## Vertex Concept

The `Vertex` concept defines requirements for vertex types:

```cpp
template <typename T>
concept Vertex = requires(unsigned binding, unsigned location) {
    { T::binding_description(binding) }
        -> std::convertible_to<vk::VertexInputBindingDescription>;
    { T::attribute_descriptions(binding, location) }
        -> std::ranges::range;
};
```

## Built-in Vertex Types

### ColoredVertex

Position with color:

```cpp
template <typename Position>
struct ColoredVertex {
    Position position;
    glm::vec3 color;
};

// Usage
using Vertex2D = ColoredVertex<glm::vec2>;
using Vertex3D = ColoredVertex<glm::vec3>;
```

### ColoredAndTexturedVertex

Position, color, and texture coordinates:

```cpp
template <typename Position>
struct ColoredAndTexturedVertex {
    Position position;
    glm::vec3 color;
    glm::vec2 uv;
};
```

### FullVertex3D

Complete 3D vertex with all PBR attributes:

```cpp
struct FullVertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 uv;
};
```

## Custom Vertex Types

### Implementation

```cpp
struct CustomVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 color;

    static vk::VertexInputBindingDescription
    binding_description(unsigned binding) {
        return {
            .binding = binding,
            .stride = sizeof(CustomVertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 4>
    attribute_descriptions(unsigned binding, unsigned location) {
        return {{
            {location + 0, binding, vk::Format::eR32G32B32Sfloat,
             offsetof(CustomVertex, position)},
            {location + 1, binding, vk::Format::eR32G32B32Sfloat,
             offsetof(CustomVertex, normal)},
            {location + 2, binding, vk::Format::eR32G32Sfloat,
             offsetof(CustomVertex, texCoord)},
            {location + 3, binding, vk::Format::eR32G32B32A32Sfloat,
             offsetof(CustomVertex, color)}
        }};
    }
};
```

### Input Rate

```cpp
// Per-vertex (default)
.inputRate = vk::VertexInputRate::eVertex

// Per-instance (for instancing)
.inputRate = vk::VertexInputRate::eInstance
```

## Attribute Formats

| GLSL Type | Vulkan Format |
|-----------|---------------|
| `float` | `eR32Sfloat` |
| `vec2` | `eR32G32Sfloat` |
| `vec3` | `eR32G32B32Sfloat` |
| `vec4` | `eR32G32B32A32Sfloat` |
| `int` | `eR32Sint` |
| `ivec2` | `eR32G32Sint` |
| `uvec4` | `eR32G32B32A32Uint` |

## Using with Pipeline

```cpp
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_vertex_binding<FullVertex3D>()
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    // ...
    .build();
```

## Multiple Bindings

```cpp
// Separate position and attributes
auto pipeline = GraphicsPipelineBuilder(device, layout)
    .add_vertex_binding<PositionVertex>()    // Binding 0
    .add_vertex_binding<AttributeVertex>()   // Binding 1
    .build();

// Bind multiple buffers
cmd.bindVertexBuffers(0, {posBuffer, attrBuffer}, {0, 0});
```

## In Shaders

```glsl
#version 450

// Match attribute locations
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec2 inTexCoord;

void main() {
    gl_Position = mvp * vec4(inPosition, 1.0);
}
```

## Best Practices

1. **Align attributes** to 4-byte boundaries
2. **Order attributes** by size (largest first)
3. **Use appropriate formats** for data range
4. **Consider interleaving** vs separate buffers
5. **Match shader locations** exactly
