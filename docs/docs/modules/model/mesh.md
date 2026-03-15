---
sidebar_position: 1
title: "Mesh & MeshManager"
---

# Mesh & MeshManager

The model module provides classes for loading 3D geometry, managing vertex/index buffers, and rendering meshes with materials. The two central classes are `MeshManager` (loads and owns geometry data) and `Mesh` (a lightweight handle to a portion of GPU buffers).

## Vertex Types

VulkanWrapper defines several vertex formats. The two most relevant for 3D rendering are:

### Vertex3D

A minimal vertex with only a position:

```cpp
struct Vertex3D : VertexInterface<glm::vec3> {
    glm::vec3 position;
};
```

Used for depth-only rendering (ZPass) where no surface attributes are needed.

### FullVertex3D

A full-featured vertex with all surface attributes:

```cpp
struct FullVertex3D
    : VertexInterface<glm::vec3, glm::vec3, glm::vec3,
                      glm::vec3, glm::vec2> {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangeant;
    glm::vec3 bitangeant;
    glm::vec2 uv;
};
```

Used for the main geometry pass (DirectLightPass) where all surface attributes are needed for lighting calculations.

### Other Vertex Types

For simpler use cases, the library also provides:

| Type | Fields |
|------|--------|
| `ColoredVertex2D` | `glm::vec2 position`, `glm::vec3 color` |
| `ColoredVertex3D` | `glm::vec3 position`, `glm::vec3 color` |
| `ColoredAndTexturedVertex2D` | `glm::vec2 position`, `glm::vec3 color`, `glm::vec2 texCoord` |
| `ColoredAndTexturedVertex3D` | `glm::vec3 position`, `glm::vec3 color`, `glm::vec2 texCoord` |

All vertex types inherit from `VertexInterface<...>`, which automatically generates Vulkan vertex input binding and attribute descriptions from the template parameters.

## Buffer Type Aliases

The model module defines type aliases for the GPU buffers that hold mesh data:

```cpp
using Vertex3DBuffer     = Buffer<Vertex3D, false, VertexBufferUsage>;
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;
```

The `false` template parameter means these are device-local (not host-visible), which gives the best GPU performance. Data is uploaded via staging buffers.

## MeshManager

`MeshManager` is the entry point for loading and managing 3D geometry. It handles:

- Loading meshes from files (OBJ, GLTF, FBX, and other formats supported by Assimp)
- Creating meshes programmatically from vertex and index arrays
- Managing GPU buffer allocation and staging uploads
- Material management through the embedded `BindlessMaterialManager`

### Constructor

```cpp
MeshManager(std::shared_ptr<const Device> device,
            std::shared_ptr<Allocator> allocator);
```

### Loading from File

```cpp
void read_file(const std::filesystem::path &path);
```

Loads all meshes from a 3D model file. Supported formats include OBJ, GLTF/GLB, FBX, and any other format supported by the Assimp library. Each sub-mesh in the file becomes a separate `Mesh` object with its own material.

```cpp
vw::Model::MeshManager mesh_manager(device, allocator);
mesh_manager.read_file("models/sponza.obj");
```

### Adding Meshes Programmatically

```cpp
void add_mesh(std::vector<FullVertex3D> vertices,
              std::vector<uint32_t> indices,
              Material::Material material);
```

Creates a mesh from raw vertex and index data with an explicit material:

```cpp
std::vector<vw::FullVertex3D> vertices = {
    {{-1, 0, -1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0}},
    {{ 1, 0, -1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0}},
    {{ 1, 0,  1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 1}},
    {{-1, 0,  1}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1}},
};
std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

auto material = colored_handler->create_material(
    vw::Model::Material::ColoredMaterialData{
        .color = glm::vec3(0.8f, 0.2f, 0.2f)});

mesh_manager.add_mesh(std::move(vertices),
                      std::move(indices),
                      material);
```

### Uploading to GPU

After adding all meshes (from files or programmatically), call `fill_command_buffer()` to stage the uploads:

```cpp
vk::CommandBuffer fill_command_buffer();
```

This returns a command buffer containing all the staging copy commands. Submit it to a queue and wait for completion before rendering:

```cpp
auto upload_cmd = mesh_manager.fill_command_buffer();
// Submit upload_cmd to a queue and wait...
```

### Accessing Meshes

```cpp
const std::vector<Mesh> &meshes() const noexcept;
```

Returns all meshes managed by this `MeshManager`. Each `Mesh` object is a lightweight handle that can be copied and stored cheaply.

### Material Manager Access

```cpp
Material::BindlessMaterialManager &material_manager() noexcept;
const Material::BindlessMaterialManager &material_manager() const noexcept;
```

Returns a reference to the embedded `BindlessMaterialManager`. Use this to register material handlers and access material data. See the [Materials](./materials.md) page for details.

## Mesh

`Mesh` is a lightweight handle representing a single drawable piece of geometry. It references shared GPU buffers (vertex buffer, index buffer) and carries material information.

### Drawing

```cpp
void draw(vk::CommandBuffer cmd_buffer,
          const PipelineLayout &pipeline_layout,
          const glm::mat4 &transform) const;
```

Draws the mesh with full vertex attributes (position, normal, tangent, bitangent, UV). Binds the full vertex buffer and index buffer, pushes `MeshPushConstants`, and issues an indexed draw call.

```cpp
void draw_zpass(vk::CommandBuffer cmd_buffer,
                const PipelineLayout &pipeline_layout,
                const glm::mat4 &transform) const;
```

Draws the mesh with only position data for the depth pre-pass. Uses the lightweight `Vertex3D` buffer.

### MeshPushConstants

Both `draw()` and `draw_zpass()` use push constants to communicate per-draw data to the shaders:

```cpp
struct MeshPushConstants {
    glm::mat4 transform;                  // Model transform matrix
    vk::DeviceAddress material_address;   // GPU address of material data
};
```

The `material_address` is a buffer device address pointing to the material's GPU data in the SSBO managed by the material handler. This enables bindless material access in shaders.

### Querying Mesh Properties

```cpp
// Material type tag for pipeline selection
Material::MaterialTypeTag material_type_tag() const noexcept;

// Number of indices in the mesh
uint32_t index_count() const noexcept;

// Access the material
const Material::Material &material() const noexcept;

// Hash for geometry deduplication (used by RayTracedScene)
size_t geometry_hash() const noexcept;
```

### Ray Tracing Support

Meshes provide methods for building acceleration structures:

```cpp
// Geometry description for BLAS building
vk::AccelerationStructureGeometryKHR
acceleration_structure_geometry() const noexcept;

// Range info for BLAS building
vk::AccelerationStructureBuildRangeInfoKHR
acceleration_structure_range_info() const noexcept;
```

These are used internally by `RayTracedScene` when building BLAS for ray tracing.

### Equality and Hashing

`Mesh` supports equality comparison and hashing based on geometry identity (not materials). Two meshes are equal if they reference the same GPU geometry:

```cpp
bool operator==(const Mesh &other) const noexcept;
size_t geometry_hash() const noexcept;
```

A `std::hash<vw::Model::Mesh>` specialization is provided, making `Mesh` usable as a key in `std::unordered_map` and `std::unordered_set`.

## Complete Example

```cpp
// Create the mesh manager
vw::Model::MeshManager mesh_manager(device, allocator);

// Register material handlers before loading
auto &mat_mgr = mesh_manager.material_manager();
mat_mgr.register_handler<
    vw::Model::Material::ColoredMaterialHandler>();
mat_mgr.register_handler<
    vw::Model::Material::TexturedMaterialHandler>(
        mat_mgr.texture_manager());

// Load a model file
mesh_manager.read_file("models/scene.gltf");

// Upload geometry and materials to GPU
auto upload_cmd = mesh_manager.fill_command_buffer();
mat_mgr.upload_all();
// Submit upload_cmd and wait...

// Access the loaded meshes
for (const auto &mesh : mesh_manager.meshes()) {
    std::cout << "Mesh with "
              << mesh.index_count() << " indices\n";
}
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/Model/MeshManager.h` | `MeshManager` |
| `VulkanWrapper/Model/Mesh.h` | `Mesh`, `MeshPushConstants`, buffer type aliases |
| `VulkanWrapper/Descriptors/Vertex.h` | `FullVertex3D`, `Vertex3D`, `ColoredVertex3D`, other vertex types |
