---
sidebar_position: 1
---

# Mesh

The mesh system handles 3D geometry loading and management.

## Overview

```cpp
#include <VulkanWrapper/Model/Mesh.h>
#include <VulkanWrapper/Model/MeshManager.h>

MeshManager meshManager(device, allocator);

// Load mesh from file
auto mesh = meshManager.load("model.obj");

// Access geometry
auto& vertices = mesh->vertices();
auto& indices = mesh->indices();
```

## MeshManager

Manages mesh loading and GPU buffer allocation.

### Constructor

```cpp
MeshManager(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator
);
```

### Loading Meshes

```cpp
// Load from file (uses Assimp)
auto mesh = meshManager.load("path/to/model.obj");
auto mesh = meshManager.load("path/to/model.gltf");
auto mesh = meshManager.load("path/to/model.fbx");

// Supported formats (via Assimp):
// - OBJ, FBX, GLTF/GLB, DAE (Collada)
// - 3DS, Blend, STL, PLY
// - And many more
```

### Creating from Data

```cpp
std::vector<FullVertex3D> vertices = /* ... */;
std::vector<uint32_t> indices = /* ... */;

auto mesh = meshManager.create(vertices, indices);
```

## Mesh Class

### Properties

| Method | Return Type | Description |
|--------|-------------|-------------|
| `vertices()` | `span<const Vertex>` | Get vertex data |
| `indices()` | `span<const uint32_t>` | Get index data |
| `vertex_buffer()` | `const VertexBuffer&` | Get GPU vertex buffer |
| `index_buffer()` | `const IndexBuffer&` | Get GPU index buffer |
| `vertex_count()` | `uint32_t` | Number of vertices |
| `index_count()` | `uint32_t` | Number of indices |
| `geometry_hash()` | `size_t` | Hash for geometry identity |

### Geometry Hash

Used to identify identical geometry for GPU data sharing:

```cpp
// Two meshes with same geometry share the hash
if (mesh1->geometry_hash() == mesh2->geometry_hash()) {
    // Can share GPU buffers
}
```

## Vertex Format

Default 3D vertex format:

```cpp
struct FullVertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 uv;
};
```

## Drawing Meshes

```cpp
// Bind buffers
cmd.bindVertexBuffers(0, {mesh->vertex_buffer().handle()}, {0});
cmd.bindIndexBuffer(mesh->index_buffer().handle(), 0,
                    vk::IndexType::eUint32);

// Draw
cmd.drawIndexed(mesh->index_count(), 1, 0, 0, 0);
```

## Importing Models

The `Importer` class provides low-level import control:

```cpp
#include <VulkanWrapper/Model/Importer.h>

Importer importer;

// Configure import
importer.setCalcTangentSpace(true);
importer.setTriangulate(true);
importer.setFlipUVs(true);

// Import
auto scene = importer.import("model.gltf");
```

### Import Options

```cpp
// Mesh processing
importer.setTriangulate(true);        // Convert to triangles
importer.setCalcTangentSpace(true);   // Generate tangents
importer.setJoinVertices(true);       // Merge identical vertices
importer.setOptimizeMeshes(true);     // Optimize for rendering
importer.setFlipUVs(true);            // Flip V coordinate
```

## Multi-Mesh Models

Some files contain multiple meshes:

```cpp
// Load all meshes from file
auto meshes = meshManager.load_all("scene.gltf");

for (auto& mesh : meshes) {
    // Process each mesh
}
```

## Memory Management

Meshes allocate GPU buffers on load:

```cpp
// GPU buffers created automatically
auto mesh = meshManager.load("model.obj");

// Buffers freed when mesh is destroyed
mesh.reset();  // Or goes out of scope
```

## Best Practices

1. **Load meshes once** and reuse them
2. **Use geometry_hash** for instancing decisions
3. **Enable tangent calculation** for normal mapping
4. **Triangulate** non-triangle meshes
5. **Consider mesh optimization** for complex models
