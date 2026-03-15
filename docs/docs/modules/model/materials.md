---
sidebar_position: 3
title: "Materials"
---

# Materials

VulkanWrapper uses a **bindless material system** where material data is stored in GPU-accessible storage buffers (SSBOs) and accessed via buffer device addresses. This design avoids per-material descriptor set switches during rendering, enabling efficient multi-material draw calls.

## Core Concepts

### Material Struct

A `Material` is a lightweight value that identifies what kind of material it is and where its GPU data lives:

```cpp
struct Material {
    MaterialTypeTag material_type;
    uint64_t buffer_address;

    Material(MaterialTypeTag type, uint64_t address);
};
```

- `material_type` -- identifies the handler that knows how to shade this material (colored, textured, emissive, etc.)
- `buffer_address` -- a `vk::DeviceAddress` pointing directly to the material's data in a GPU storage buffer

During rendering, the `buffer_address` is pushed as part of `MeshPushConstants`, and the shader reads material data directly from the buffer using the address.

### MaterialTypeTag

`MaterialTypeTag` is a strong type wrapping a `uint32_t` identifier. Each material handler type has a unique tag, registered via macros:

```cpp
class MaterialTypeTag {
public:
    constexpr explicit MaterialTypeTag(MaterialTypeId id) noexcept;
    constexpr MaterialTypeId id() const noexcept;
    auto operator<=>(const MaterialTypeTag &) const = default;
};
```

Tags are declared and defined in pairs:

```cpp
// In header (.h):
VW_DECLARE_MATERIAL_TYPE(my_custom_material_tag);

// In source (.cpp):
VW_DEFINE_MATERIAL_TYPE(my_custom_material_tag);
```

Each `VW_DEFINE_MATERIAL_TYPE` call assigns the next available ID from a global counter. Tags are used to:

- Select the correct rendering pipeline per material type (via `MeshRenderer`)
- Map materials to shader binding table offsets for ray tracing

A `std::hash<MaterialTypeTag>` specialization is provided, so tags can be used as keys in `std::unordered_map`.

### MaterialPriority

Each handler has a priority that determines the order in which material types are tried when importing from a file:

```cpp
enum class MaterialPriority {};

constexpr MaterialPriority colored_material_priority = MaterialPriority{0};
constexpr MaterialPriority textured_material_priority = MaterialPriority{1};
constexpr MaterialPriority emissive_textured_material_priority = MaterialPriority{2};
constexpr MaterialPriority user_material_priority = MaterialPriority{1024};
```

When loading a model file, handlers are tried in priority order. The first handler that successfully creates GPU data from the `aiMaterial` wins.

## BindlessMaterialManager

`BindlessMaterialManager` is the central registry for all material handlers. It manages handler registration, material creation from imported files, and provides the data needed for shader binding tables.

### Constructor

```cpp
BindlessMaterialManager(
    std::shared_ptr<const Device> device,
    std::shared_ptr<Allocator> allocator,
    std::shared_ptr<StagingBufferManager> staging);
```

Typically you do not construct this directly -- `MeshManager` creates one internally and exposes it via `material_manager()`.

### Registering Handlers

```cpp
template <typename Handler, typename... Args>
void register_handler(Args &&...args);
```

Register a material handler before loading any model files. The handler is constructed internally using the `MaterialTypeHandler::create<Handler>()` factory:

```cpp
auto &mat_mgr = mesh_manager.material_manager();

// Register built-in handlers
mat_mgr.register_handler<
    vw::Model::Material::ColoredMaterialHandler>();

mat_mgr.register_handler<
    vw::Model::Material::TexturedMaterialHandler>(
        mat_mgr.texture_manager());

mat_mgr.register_handler<
    vw::Model::Material::EmissiveTexturedMaterialHandler>(
        mat_mgr.texture_manager());
```

Additional arguments after the handler type are forwarded to the handler's constructor (after `device` and `allocator`, which are provided automatically).

### Creating Materials from Files

```cpp
Material create_material(const aiMaterial *mat,
                         const std::filesystem::path &base_path);
```

This is called internally by `MeshManager::read_file()`. It tries each registered handler in priority order until one succeeds at creating GPU data from the Assimp material.

### Uploading to GPU

```cpp
void upload_all();
```

Uploads all pending material data to GPU buffers. Call this after loading all meshes and before rendering.

### Accessing Handlers

```cpp
// Get a specific handler by tag
IMaterialTypeHandler *handler(MaterialTypeTag tag);
const IMaterialTypeHandler *handler(MaterialTypeTag tag) const;

// Iterate all handlers (unordered)
auto handlers() const;

// Get handlers in deterministic order (sorted by tag)
std::vector<std::pair<MaterialTypeTag, const IMaterialTypeHandler *>>
ordered_handlers() const;

// Get material-to-SBT-offset mapping for ray tracing
std::unordered_map<MaterialTypeTag, uint32_t> sbt_mapping() const;
```

The `sbt_mapping()` method returns a map from each `MaterialTypeTag` to its index in the deterministic handler order. This is used by `RayTracedScene` to assign SBT (Shader Binding Table) offsets per material type.

### Texture Manager Access

```cpp
BindlessTextureManager &texture_manager() noexcept;
const BindlessTextureManager &texture_manager() const noexcept;
```

The `BindlessTextureManager` handles loading, caching, and descriptor management for textures. It is used by `TexturedMaterialHandler` and `EmissiveTexturedMaterialHandler`.

## Built-in Material Handlers

### IMaterialTypeHandler (Interface)

All handlers implement this interface:

```cpp
class IMaterialTypeHandler {
public:
    virtual MaterialTypeTag tag() const = 0;
    virtual MaterialPriority priority() const = 0;

    virtual std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) = 0;

    virtual vk::DeviceAddress buffer_address() const = 0;
    virtual uint32_t stride() const = 0;
    virtual void upload() = 0;

    virtual std::vector<Barrier::ResourceState>
    get_resources() const = 0;

    // Optional: additional descriptor set for textures
    virtual std::optional<vk::DescriptorSet>
    additional_descriptor_set() const;

    virtual std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const;

    const std::filesystem::path &brdf_path() const;
};
```

### MaterialTypeHandler\<GpuData\> (Template Base)

The `MaterialTypeHandler<GpuData>` template provides the common SSBO-based storage for material data. It manages a `Buffer<GpuData, true, StorageBufferUsage>` that grows as materials are added:

```cpp
template <typename GpuData>
class MaterialTypeHandler : public IMaterialTypeHandler {
public:
    // Create a material from explicit GPU data
    Material create_material(GpuData data);
};
```

### ColoredMaterialHandler

Handles solid-color materials. GPU data:

```cpp
struct ColoredMaterialData {
    glm::vec3 color;
};
```

Registration and usage:

```cpp
mat_mgr.register_handler<
    vw::Model::Material::ColoredMaterialHandler>();

// Access the handler to create materials programmatically
auto *handler = mat_mgr.handler(
    vw::Model::Material::colored_material_tag);
auto *colored = static_cast<
    vw::Model::Material::ColoredMaterialHandler *>(handler);

// Or use the template base's create_material:
auto material = colored->create_material(
    vw::Model::Material::ColoredMaterialData{
        .color = glm::vec3(0.9f, 0.1f, 0.1f)});
```

### TexturedMaterialHandler

Handles textured materials with a diffuse texture. GPU data:

```cpp
struct TexturedMaterialData {
    uint32_t diffuse_texture_index;
};
```

Registration requires passing the `BindlessTextureManager`:

```cpp
mat_mgr.register_handler<
    vw::Model::Material::TexturedMaterialHandler>(
        mat_mgr.texture_manager());
```

Creating a textured material programmatically:

```cpp
auto *handler = static_cast<
    vw::Model::Material::TexturedMaterialHandler *>(
        mat_mgr.handler(
            vw::Model::Material::textured_material_tag));

auto material = handler->create_material(
    std::filesystem::path("textures/brick.png"));
```

The `TexturedMaterialHandler` also provides an `additional_descriptor_set()` containing the bindless texture array, which is bound as an extra descriptor set during rendering.

### EmissiveTexturedMaterialHandler

Handles emissive textured materials (e.g., glowing surfaces, light panels). GPU data:

```cpp
struct EmissiveTexturedMaterialData {
    uint32_t diffuse_texture_index;
    float intensity;  // luminance in lumen/m^2
};
```

Registration:

```cpp
mat_mgr.register_handler<
    vw::Model::Material::EmissiveTexturedMaterialHandler>(
        mat_mgr.texture_manager());
```

Creating an emissive material:

```cpp
auto *handler = static_cast<
    vw::Model::Material::EmissiveTexturedMaterialHandler *>(
        mat_mgr.handler(
            vw::Model::Material::emissive_textured_material_tag));

auto material = handler->create_material(
    std::filesystem::path("textures/neon.png"),
    500.0f);  // intensity in lumen/m^2
```

## Creating a Custom Material Handler

To add a new material type:

1. **Define the GPU data struct**:

```cpp
struct MyMaterialData {
    glm::vec3 base_color;
    float roughness;
    float metallic;
};
```

2. **Declare and define the tag**:

```cpp
// In header:
VW_DECLARE_MATERIAL_TYPE(my_material_tag);

// In source:
VW_DEFINE_MATERIAL_TYPE(my_material_tag);
```

3. **Create the handler class**:

```cpp
class MyMaterialHandler
    : public vw::Model::Material::MaterialTypeHandler<MyMaterialData> {
    friend class MaterialTypeHandler<MyMaterialData>;

public:
    using Base = MaterialTypeHandler<MyMaterialData>;

    MaterialTypeTag tag() const override {
        return my_material_tag;
    }

    MaterialPriority priority() const override {
        return user_material_priority;
    }

protected:
    std::optional<MyMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override {
        // Extract data from Assimp material...
        // Return std::nullopt if this handler cannot handle it
        return MyMaterialData{
            .base_color = glm::vec3(1.0f),
            .roughness = 0.5f,
            .metallic = 0.0f};
    }

private:
    MyMaterialHandler(std::shared_ptr<const Device> device,
                      std::shared_ptr<Allocator> allocator)
        : Base(std::move(device), std::move(allocator),
               "shaders/my_brdf.glsl") {}
};
```

4. **Register the handler**:

```cpp
mat_mgr.register_handler<MyMaterialHandler>();
```

5. **Write the corresponding BRDF shader** referenced by `brdf_path()`.

The `MaterialTypeHandler<GpuData>` base class automatically handles:
- SSBO allocation and growth
- Material data storage and `buffer_address()` computation
- Upload to GPU via `upload()`
- Resource state tracking for barrier management

## Complete Example

```cpp
// Set up mesh manager with materials
vw::Model::MeshManager mesh_manager(device, allocator);
auto &mat_mgr = mesh_manager.material_manager();

// Register handlers
mat_mgr.register_handler<
    vw::Model::Material::ColoredMaterialHandler>();
mat_mgr.register_handler<
    vw::Model::Material::TexturedMaterialHandler>(
        mat_mgr.texture_manager());
mat_mgr.register_handler<
    vw::Model::Material::EmissiveTexturedMaterialHandler>(
        mat_mgr.texture_manager());

// Load model (materials are auto-detected from the file)
mesh_manager.read_file("models/scene.gltf");

// Create a programmatic material
auto *colored = static_cast<
    vw::Model::Material::ColoredMaterialHandler *>(
        mat_mgr.handler(
            vw::Model::Material::colored_material_tag));
auto red_material = colored->create_material(
    vw::Model::Material::ColoredMaterialData{
        .color = glm::vec3(1, 0, 0)});

// Add a custom mesh with the red material
mesh_manager.add_mesh(my_vertices, my_indices, red_material);

// Upload everything to GPU
auto upload_cmd = mesh_manager.fill_command_buffer();
mat_mgr.upload_all();
// Submit upload_cmd and wait...

// For ray tracing: get SBT mapping
auto sbt_map = mat_mgr.sbt_mapping();
ray_traced_scene.set_material_sbt_mapping(sbt_map);
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/Model/Material/Material.h` | `Material` struct |
| `VulkanWrapper/Model/Material/MaterialTypeTag.h` | `MaterialTypeTag`, `VW_DECLARE_MATERIAL_TYPE`, `VW_DEFINE_MATERIAL_TYPE` |
| `VulkanWrapper/Model/Material/MaterialPriority.h` | `MaterialPriority` constants |
| `VulkanWrapper/Model/Material/BindlessMaterialManager.h` | `BindlessMaterialManager` |
| `VulkanWrapper/Model/Material/IMaterialTypeHandler.h` | `IMaterialTypeHandler` interface |
| `VulkanWrapper/Model/Material/MaterialTypeHandler.h` | `MaterialTypeHandler<GpuData>` template |
| `VulkanWrapper/Model/Material/ColoredMaterialHandler.h` | `ColoredMaterialHandler` |
| `VulkanWrapper/Model/Material/TexturedMaterialHandler.h` | `TexturedMaterialHandler` |
| `VulkanWrapper/Model/Material/EmissiveTexturedMaterialHandler.h` | `EmissiveTexturedMaterialHandler` |
| `VulkanWrapper/Model/Material/MaterialData.h` | `ColoredMaterialData`, `TexturedMaterialData`, `EmissiveTexturedMaterialData` |
