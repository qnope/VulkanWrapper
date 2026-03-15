---
sidebar_position: 1
title: "Acceleration Structures"
---

# Acceleration Structures

VulkanWrapper provides `RayTracedScene` as a high-level manager for Vulkan ray tracing acceleration structures. It handles the complexities of BLAS (Bottom-Level Acceleration Structure) and TLAS (Top-Level Acceleration Structure) building, geometry deduplication, instance management, and SBT (Shader Binding Table) offset assignment.

## Background: BLAS and TLAS

Vulkan ray tracing uses a two-level acceleration structure hierarchy:

- **BLAS (Bottom-Level)**: Contains the actual triangle geometry for a single mesh. Building a BLAS is expensive, so the same BLAS is reused for all instances of a mesh.
- **TLAS (Top-Level)**: Contains references to BLAS instances with per-instance transforms. The TLAS is what the ray tracing pipeline traces rays against.

`RayTracedScene` manages both levels automatically. When you add an instance of a mesh, it:

1. Checks if a BLAS for that mesh's geometry already exists (deduplication via `Mesh::geometry_hash()`).
2. Creates a new BLAS only if needed.
3. Adds a TLAS instance referencing the BLAS with the provided transform.

## InstanceId

Each instance in the scene is identified by an `InstanceId`:

```cpp
struct InstanceId {
    uint32_t value;
    bool operator==(const InstanceId &other) const noexcept = default;
};
```

`InstanceId` is returned by `add_instance()` and used to modify or remove instances later. Store it to update transforms, visibility, or SBT offsets at runtime.

## RayTracedScene

### Constructor

```cpp
RayTracedScene(std::shared_ptr<const Device> device,
               std::shared_ptr<const Allocator> allocator);
```

### Adding Instances

```cpp
InstanceId add_instance(
    const Model::Mesh &mesh,
    const glm::mat4 &transform = glm::mat4(1.0f));
```

Adds a mesh instance to the scene. The mesh geometry is automatically registered if not already known (deduplication via geometry hash). The mesh is also added to the embedded `Scene` for rasterization rendering.

```cpp
vw::rt::RayTracedScene scene(device, allocator);

auto id1 = scene.add_instance(floor_mesh);
auto id2 = scene.add_instance(cube_mesh,
    glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, 0)));
auto id3 = scene.add_instance(cube_mesh,
    glm::translate(glm::mat4(1.0f), glm::vec3(3, 1, 0)));
```

In the example above, `cube_mesh` is added twice with different transforms. Only one BLAS is built for the cube geometry -- both instances share it.

### Modifying Instances

```cpp
// Update the transform of an instance
void set_transform(InstanceId instance_id,
                   const glm::mat4 &transform);

// Get the current transform
const glm::mat4 &get_transform(InstanceId instance_id) const;

// Show or hide an instance (hidden instances are excluded from TLAS)
void set_visible(InstanceId instance_id, bool visible);
bool is_visible(InstanceId instance_id) const;

// Set the SBT offset for this instance
void set_sbt_offset(InstanceId instance_id, uint32_t offset);
uint32_t get_sbt_offset(InstanceId instance_id) const;

// Set a custom index accessible in shaders via
// gl_InstanceCustomIndexEXT
void set_custom_index(InstanceId instance_id,
                      uint32_t custom_index);
uint32_t get_custom_index(InstanceId instance_id) const;

// Check if an instance ID is still valid
bool is_valid(InstanceId instance_id) const;
```

### Removing Instances

```cpp
void remove_instance(InstanceId instance_id);
```

Marks the instance as inactive. After removing, call `update()` to rebuild the TLAS.

### Material SBT Mapping

```cpp
void set_material_sbt_mapping(
    std::unordered_map<Model::Material::MaterialTypeTag, uint32_t>
        mapping);
```

Sets the mapping from material types to SBT (Shader Binding Table) offsets. This determines which closest-hit shader group is invoked for each material type during ray tracing. Typically, you get this mapping from `BindlessMaterialManager::sbt_mapping()`:

```cpp
auto sbt_map = material_manager.sbt_mapping();
ray_traced_scene.set_material_sbt_mapping(sbt_map);
```

When this mapping is set, `add_instance()` automatically assigns the correct SBT offset based on the mesh's material type.

### Building Acceleration Structures

```cpp
// Build BLAS + TLAS (call after adding all initial instances)
void build();

// Rebuild TLAS only (for transform/visibility changes)
void update();
```

`build()` performs the full acceleration structure build:

1. Builds a BLAS for each unique mesh geometry.
2. Builds the TLAS from all visible instances.
3. Builds the geometry reference buffer for shader access.

`update()` rebuilds only the TLAS, which is much faster. Use it when transforms or visibility have changed but no new geometry has been added.

```cpp
// Initial setup
scene.add_instance(mesh_a);
scene.add_instance(mesh_b, some_transform);
scene.build();

// Later: update a transform
scene.set_transform(id2, new_transform);
scene.update();
```

### Querying Build State

```cpp
bool needs_build() const noexcept;   // New geometry added?
bool needs_update() const noexcept;  // Transforms changed?
```

### Accessing Acceleration Structures

```cpp
// TLAS handle for descriptor binding
vk::AccelerationStructureKHR tlas_handle() const;

// TLAS device address for shader access
vk::DeviceAddress tlas_device_address() const;

// Full TLAS object reference
const as::TopLevelAccelerationStructure &tlas() const;

// Geometry buffer for ray tracing shaders
vk::DeviceAddress geometry_buffer_address() const;
const GeometryReferenceBuffer &geometry_buffer() const;
bool has_geometry_buffer() const noexcept;
```

### Scene Counts

```cpp
size_t mesh_count() const noexcept;             // Unique geometries
size_t instance_count() const noexcept;          // Total instances
size_t visible_instance_count() const noexcept;  // Visible instances
```

### Embedded Scene for Rasterization

`RayTracedScene` maintains an embedded `Scene` that mirrors its instance data for rasterization:

```cpp
const Model::Scene &scene() const noexcept;
Model::Scene &scene() noexcept;
```

When you add an instance to `RayTracedScene`, it is automatically added to the internal `Scene`. This means you can use the same `RayTracedScene` for both ray tracing and rasterization passes without maintaining separate scene representations.

## GeometryReference

The geometry reference buffer stores per-BLAS metadata that ray tracing shaders need to look up vertex data and material information:

```cpp
struct GeometryReference {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
    int32_t vertex_offset;
    int32_t first_index;
    uint32_t material_type;
    uint32_t _padding;
    uint64_t material_address;
    glm::mat4 matrix;
};
static_assert(sizeof(GeometryReference) == 104);
```

In a closest-hit shader, you use `gl_GeometryIndexEXT` to index into this buffer and retrieve the vertex/index buffer addresses and material data for the hit triangle.

```cpp
using GeometryReferenceBuffer =
    Buffer<GeometryReference, true, StorageBufferUsage>;
```

## Complete Example

```cpp
// Create mesh manager and load geometry
vw::Model::MeshManager mesh_manager(device, allocator);
auto &mat_mgr = mesh_manager.material_manager();
mat_mgr.register_handler<
    vw::Model::Material::ColoredMaterialHandler>();
mat_mgr.register_handler<
    vw::Model::Material::TexturedMaterialHandler>(
        mat_mgr.texture_manager());

mesh_manager.read_file("models/scene.gltf");
auto upload_cmd = mesh_manager.fill_command_buffer();
mat_mgr.upload_all();
// Submit upload_cmd and wait...

// Create ray traced scene
vw::rt::RayTracedScene rt_scene(device, allocator);

// Set material SBT mapping before adding instances
rt_scene.set_material_sbt_mapping(mat_mgr.sbt_mapping());

// Add all meshes as instances
for (const auto &mesh : mesh_manager.meshes()) {
    rt_scene.add_instance(mesh);
}

// Build acceleration structures
rt_scene.build();

// Use in render passes
z_pass.set_scene(rt_scene);
auto &direct = pipeline.add(
    std::make_unique<vw::DirectLightPass>(
        device, allocator, shader_dir,
        rt_scene, mat_mgr));

// In the render loop, if transforms changed:
rt_scene.set_transform(some_id, new_transform);
rt_scene.update();
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/RayTracing/RayTracedScene.h` | `RayTracedScene`, `InstanceId` |
| `VulkanWrapper/RayTracing/GeometryReference.h` | `GeometryReference`, `GeometryReferenceBuffer` |
| `VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h` | BLAS types (used internally) |
| `VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h` | TLAS types (used internally) |
