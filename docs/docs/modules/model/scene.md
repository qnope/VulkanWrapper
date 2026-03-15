---
sidebar_position: 2
title: "Scene"
---

# Scene

The `Scene` class manages a collection of mesh instances -- meshes placed in the world with individual transforms. It enables **instanced rendering**, where the same mesh geometry is reused multiple times at different positions, rotations, and scales without duplicating GPU buffer data.

## MeshInstance

A `MeshInstance` pairs a `Mesh` with a transform matrix:

```cpp
struct MeshInstance {
    Mesh mesh;
    glm::mat4 transform = glm::mat4(1.0f);
};
```

The `Mesh` object is copied, but this is cheap because `Mesh` internally holds `shared_ptr` references to the underlying GPU buffers. The `transform` is a 4x4 model matrix that positions the mesh in world space. It defaults to the identity matrix (no transformation).

## Scene Class

```cpp
class Scene {
public:
    Scene() = default;

    void add_mesh_instance(const Mesh &mesh);
    void add_mesh_instance(const Mesh &mesh,
                           const glm::mat4 &transform);

    const std::vector<MeshInstance> &instances() const noexcept;
    std::vector<MeshInstance> &instances() noexcept;

    void clear() noexcept;
    size_t size() const noexcept;
};
```

### Adding Instances

There are two overloads for adding a mesh to the scene:

```cpp
// Add with identity transform (no translation/rotation/scale)
void add_mesh_instance(const Mesh &mesh);

// Add with a specific transform
void add_mesh_instance(const Mesh &mesh,
                       const glm::mat4 &transform);
```

You can add the same mesh multiple times with different transforms to create multiple instances:

```cpp
vw::Model::Scene scene;

// Place the same mesh at three different positions
auto translate = [](float x, float y, float z) {
    return glm::translate(glm::mat4(1.0f),
                          glm::vec3(x, y, z));
};

scene.add_mesh_instance(tree_mesh, translate(0, 0, 0));
scene.add_mesh_instance(tree_mesh, translate(5, 0, 0));
scene.add_mesh_instance(tree_mesh, translate(0, 0, 8));
```

### Accessing Instances

```cpp
// Read-only access
const std::vector<MeshInstance> &instances() const noexcept;

// Mutable access (for modifying transforms at runtime)
std::vector<MeshInstance> &instances() noexcept;
```

The mutable overload lets you update transforms at runtime without removing and re-adding instances:

```cpp
// Rotate the first instance over time
auto &inst = scene.instances()[0];
inst.transform = glm::rotate(
    inst.transform, angle, glm::vec3(0, 1, 0));
```

### Clearing and Querying

```cpp
// Remove all instances
void clear() noexcept;

// Get the number of instances
size_t size() const noexcept;
```

## Scene and RayTracedScene

In practice, you will often use `RayTracedScene` instead of `Scene` directly. `RayTracedScene` embeds a `Scene` internally and keeps it synchronized with the acceleration structure hierarchy:

```cpp
// RayTracedScene provides access to its embedded Scene:
const Model::Scene &scene() const noexcept;
Model::Scene &scene() noexcept;
```

When you call `ray_traced_scene.add_instance(mesh, transform)`, the mesh instance is added to both the ray tracing acceleration structures and the internal `Scene` for rasterization. This means you get both rasterization and ray tracing support from a single source of truth.

See the [Acceleration Structures](../raytracing/acceleration-structures.md) page for details on `RayTracedScene`.

## Rendering a Scene

The `DirectLightPass` and `ZPass` iterate over the scene's instances to draw them. Here is a simplified view of what happens internally:

```cpp
// Conceptual rendering loop (simplified)
for (const auto &instance : scene.instances()) {
    instance.mesh.draw(cmd, pipeline_layout,
                       instance.transform);
}
```

Each mesh binds its vertex/index buffers and pushes `MeshPushConstants` (transform + material address) before issuing an indexed draw call.

## Instancing Patterns

### Loading All Meshes from a File

When you load a model file with `MeshManager`, each sub-mesh becomes a separate `Mesh`. You can add all of them to a scene at once:

```cpp
mesh_manager.read_file("models/scene.gltf");
auto upload_cmd = mesh_manager.fill_command_buffer();
// Submit and wait...

vw::Model::Scene scene;
for (const auto &mesh : mesh_manager.meshes()) {
    scene.add_mesh_instance(mesh);
}
```

### Procedural Placement

You can place meshes procedurally using computed transforms:

```cpp
// Create a grid of cubes
for (int x = 0; x < 10; ++x) {
    for (int z = 0; z < 10; ++z) {
        auto transform = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(x * 2.0f, 0.0f, z * 2.0f));
        scene.add_mesh_instance(cube_mesh, transform);
    }
}
```

### Dynamic Scenes

For scenes that change over time, you can clear and rebuild:

```cpp
scene.clear();
for (const auto &entity : game_entities) {
    scene.add_mesh_instance(entity.mesh,
                            entity.world_transform());
}
```

Or modify individual instance transforms in place:

```cpp
scene.instances()[player_index].transform =
    player.world_transform();
```

## Key Headers

| Header | Contents |
|--------|----------|
| `VulkanWrapper/Model/Scene.h` | `Scene`, `MeshInstance` |
| `VulkanWrapper/Model/Mesh.h` | `Mesh` (referenced by `MeshInstance`) |
