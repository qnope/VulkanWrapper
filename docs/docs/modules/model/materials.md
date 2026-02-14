---
sidebar_position: 3
---

# Materials

VulkanWrapper provides an extensible material system with bindless texture support.

## Overview

```cpp
#include <VulkanWrapper/Model/Material/Material.h>
#include <VulkanWrapper/Model/Material/BindlessMaterialManager.h>

BindlessMaterialManager materialManager(device, allocator);

// Register material handlers
materialManager.register_handler<ColoredMaterialHandler>();
materialManager.register_handler<TexturedMaterialHandler>();

// Create materials
auto colorMat = materialManager.create_colored({1, 0, 0, 1});
auto texMat = materialManager.create_textured(albedoTexture, normalTexture);
```

## Material Structure

Materials are lightweight references containing a type tag and a GPU buffer address:

```cpp
struct Material {
    MaterialTypeTag material_type;  // Type identifier
    uint64_t buffer_address;        // GPU device address of material data
};
```

### MaterialTypeTag

Identifies the material type:

```cpp
struct MaterialTypeTag {
    uint32_t value;

    static MaterialTypeTag create(const char* name);
};

// Usage
auto coloredTag = MaterialTypeTag::create("colored");
auto texturedTag = MaterialTypeTag::create("textured");
```

## BindlessMaterialManager

Central manager for all materials.

### Constructor

```cpp
BindlessMaterialManager(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator
);
```

### Registering Handlers

```cpp
// Register built-in handlers
materialManager.register_handler<ColoredMaterialHandler>();
materialManager.register_handler<TexturedMaterialHandler>();

// Register custom handler
materialManager.register_handler<MyCustomMaterialHandler>();
```

### Creating Materials

```cpp
// Colored material
auto redMaterial = materialManager.create_colored(
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
);

// Textured material
auto texMaterial = materialManager.create_textured(
    albedoTexture,
    normalTexture,
    metallicRoughnessTexture
);
```

### Binding

```cpp
// Get descriptor set for material type
auto descriptorSet = materialManager.get_descriptor_set(material.type);

// Bind in shader
cmd.bindDescriptorSets(bindPoint, layout, setIndex, descriptorSet, {});

// Material address is available directly via buffer_address
// Push via push constants or buffer device address
```

## Material Handlers

### IMaterialTypeHandler Interface

```cpp
class IMaterialTypeHandler {
public:
    virtual MaterialTypeTag type_tag() const = 0;
    virtual MaterialPriority priority() const = 0;

    virtual void bind(CommandBuffer& cmd,
                      const PipelineLayout& layout,
                      uint32_t setIndex) const = 0;

    virtual DescriptorSetLayout& descriptor_layout() const = 0;
};
```

### ColoredMaterialHandler

Simple solid color materials:

```cpp
struct ColoredMaterialData {
    glm::vec4 color;
};

class ColoredMaterialHandler : public MaterialTypeHandler<ColoredMaterialData> {
public:
    Material create(const glm::vec4& color);
};
```

Usage in shader:
```glsl
layout(set = 1, binding = 0) buffer Materials {
    vec4 colors[];
};

layout(push_constant) uniform PC {
    uint materialIndex;
};

void main() {
    vec4 color = colors[materialIndex];
}
```

### TexturedMaterialHandler

PBR textured materials:

```cpp
struct TexturedMaterialData {
    uint32_t albedoIndex;
    uint32_t normalIndex;
    uint32_t metallicRoughnessIndex;
};

class TexturedMaterialHandler : public MaterialTypeHandler<TexturedMaterialData> {
public:
    Material create(const CombinedImage& albedo,
                    const CombinedImage& normal,
                    const CombinedImage& metallicRoughness);
};
```

Usage in shader:
```glsl
layout(set = 1, binding = 0) buffer Materials {
    uvec4 textureIndices[];  // albedo, normal, metallicRoughness, padding
};

layout(set = 1, binding = 1) uniform sampler2D textures[];

void main() {
    uvec4 indices = textureIndices[materialIndex];
    vec4 albedo = texture(textures[indices.x], uv);
    vec3 normal = texture(textures[indices.y], uv).xyz;
    vec2 mr = texture(textures[indices.z], uv).xy;
}
```

## BindlessTextureManager

Manages bindless texture arrays:

```cpp
#include <VulkanWrapper/Model/Material/BindlessTextureManager.h>

BindlessTextureManager textures(device, allocator, maxTextures);

// Add textures
uint32_t index = textures.add(imageView, sampler);

// Get descriptor set
auto descriptorSet = textures.descriptor_set();
```

## Custom Material Types

### Implementing a Handler

```cpp
struct EmissiveMaterialData {
    glm::vec4 color;
    float intensity;
    uint32_t padding[3];
};

class EmissiveMaterialHandler
    : public MaterialTypeHandler<EmissiveMaterialData> {
public:
    EmissiveMaterialHandler(std::shared_ptr<const Device> device,
                            std::shared_ptr<const Allocator> allocator)
        : MaterialTypeHandler(device, allocator) {}

    MaterialTypeTag type_tag() const override {
        return MaterialTypeTag::create("emissive");
    }

    MaterialPriority priority() const override {
        return MaterialPriority::Transparent;  // Render after opaque
    }

    Material create(const glm::vec4& color, float intensity) {
        EmissiveMaterialData data{color, intensity, {}};
        return add_material(data);
    }
};
```

### Registration

```cpp
materialManager.register_handler<EmissiveMaterialHandler>();
auto glowMat = materialManager.create<EmissiveMaterialHandler>(
    glm::vec4(1, 0.5, 0, 1), 10.0f
);
```

## Material Priority

Controls render order:

```cpp
enum class MaterialPriority {
    Opaque = 0,      // Render first
    AlphaTest = 1,   // After opaque
    Transparent = 2  // Render last, back-to-front
};
```

## Best Practices

1. **Use bindless textures** for many materials
2. **Group by material type** for efficient batching
3. **Consider material priority** for transparency
4. **Pack material data** efficiently
5. **Use buffer device addresses** for material data access
