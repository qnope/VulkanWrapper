---
sidebar_position: 2
---

# Scene

The `Scene` class manages collections of mesh instances with transforms.

## Overview

```cpp
#include <VulkanWrapper/Model/Scene.h>

Scene scene;

// Add mesh instances
scene.add(mesh1, transform1);
scene.add(mesh1, transform2);  // Same mesh, different position
scene.add(mesh2, transform3);

// Iterate instances
for (const auto& instance : scene.instances()) {
    render(instance.mesh, instance.transform);
}
```

## Scene Class

### Adding Instances

```cpp
// Add with identity transform
scene.add(mesh);

// Add with transform
scene.add(mesh, glm::translate(glm::mat4(1), glm::vec3(5, 0, 0)));

// Add MeshInstance directly
MeshInstance instance{mesh, transform};
scene.add(instance);
```

### Accessing Instances

```cpp
// Get all instances
const auto& instances = scene.instances();

// Iterate
for (const auto& inst : scene.instances()) {
    auto& mesh = inst.mesh;
    auto& transform = inst.transform;
}

// Instance count
size_t count = scene.instance_count();
```

### Clearing

```cpp
// Remove all instances
scene.clear();
```

## MeshInstance

Pairs a mesh with a transform:

```cpp
struct MeshInstance {
    std::shared_ptr<Mesh> mesh;
    glm::mat4 transform;
};
```

### Creating Instances

```cpp
// Direct construction
MeshInstance instance{
    .mesh = mesh,
    .transform = glm::translate(glm::mat4(1), position)
};

// With rotation and scale
glm::mat4 transform = glm::mat4(1);
transform = glm::translate(transform, position);
transform = glm::rotate(transform, angle, axis);
transform = glm::scale(transform, scale);

MeshInstance instance{mesh, transform};
```

## Rendering a Scene

```cpp
void renderScene(CommandBuffer& cmd, const Scene& scene,
                 const Pipeline& pipeline) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                     pipeline.handle());

    for (const auto& instance : scene.instances()) {
        // Update transform
        PushConstants pc{
            .model = instance.transform,
            .viewProj = camera.viewProj()
        };
        cmd.pushConstants(layout, stages, 0, sizeof(pc), &pc);

        // Bind mesh buffers
        cmd.bindVertexBuffers(0, {instance.mesh->vertex_buffer().handle()},
                              {0});
        cmd.bindIndexBuffer(instance.mesh->index_buffer().handle(), 0,
                            vk::IndexType::eUint32);

        // Draw
        cmd.drawIndexed(instance.mesh->index_count(), 1, 0, 0, 0);
    }
}
```

## Instanced Rendering

For many instances of the same mesh:

```cpp
// Group by mesh
std::map<size_t, std::vector<glm::mat4>> instancesByMesh;

for (const auto& inst : scene.instances()) {
    auto hash = inst.mesh->geometry_hash();
    instancesByMesh[hash].push_back(inst.transform);
}

// Render with instancing
for (const auto& [hash, transforms] : instancesByMesh) {
    auto mesh = /* find mesh by hash */;

    // Upload transforms to instance buffer
    instanceBuffer.write(transforms, 0);

    // Draw all instances
    cmd.drawIndexed(mesh->index_count(), transforms.size(), 0, 0, 0);
}
```

## Ray Tracing Integration

Build acceleration structures from scene:

```cpp
#include <VulkanWrapper/RayTracing/RayTracedScene.h>

RayTracedScene rtScene(device, allocator);

// Add scene instances
for (const auto& inst : scene.instances()) {
    rtScene.add_instance(inst.mesh, inst.transform);
}

// Build TLAS
rtScene.build(cmd);
```

## Scene Updates

For dynamic scenes:

```cpp
class DynamicScene {
public:
    void update(float deltaTime) {
        for (auto& instance : m_instances) {
            // Update transforms
            instance.transform = computeNewTransform(instance, deltaTime);
        }

        // Rebuild TLAS for ray tracing
        m_rtScene.update(cmd, getInstanceTransforms());
    }
};
```

## Loading Scenes

Load complete scenes from files:

```cpp
// Some formats contain scene hierarchy
auto sceneData = importer.import("scene.gltf");

Scene scene;
for (const auto& node : sceneData.nodes) {
    scene.add(node.mesh, node.transform);
}
```

## Best Practices

1. **Group by mesh** for efficient rendering
2. **Use instancing** for repeated meshes
3. **Update transforms** through scene interface
4. **Rebuild TLAS** when transforms change
5. **Consider spatial partitioning** for large scenes
