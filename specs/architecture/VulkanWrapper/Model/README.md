# Model Module

`vw::Model` namespace · `Model/` directory · Tier 5

3D model loading, mesh management, and scene representation with bindless materials.

## MeshManager

Central entry point for loading and managing meshes:

```cpp
vw::Model::MeshManager mesh_manager(device, allocator);
mesh_manager.read_file("Models/Sponza/Sponza.gltf");
auto cmd = mesh_manager.fill_command_buffer();  // upload to GPU via staging
queue.enqueue_command_buffer(cmd);
queue.submit({}, {}, {}).wait();
```

- `add_mesh(vertices, indices, material)` — add programmatic mesh
- `meshes()` — access loaded meshes
- `material_manager()` — access the `BindlessMaterialManager`
- Internally manages vertex buffers (`Vertex3D` + `FullVertex3D`), index buffers via `BufferList`

## Mesh

Single submesh with vertex/index buffer references and material. Hashable for use as map key (geometry deduplication in RayTracedScene).

## Scene

Collection of `MeshInstance` (mesh + transform matrix):

```cpp
Scene scene;
scene.add_mesh_instance(mesh, glm::mat4(1.0f));
for (auto& inst : scene.instances()) { /* render */ }
```

Used by both rasterization (MeshRenderer) and ray tracing (RayTracedScene embeds a Scene).

## Importer

Assimp-based model file loader. Auto-detects format, extracts submeshes with materials.

## Internal/

`MaterialInfo`, `MeshInfo` — internal implementation details for model import.

See [Material](Material/README.md) for the material subsystem.
