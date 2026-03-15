# Task 2: Common Scene Setup Header

## Summary

Create a shared header `examples/Common/SceneSetup.h` providing inline helper functions for building the cube+plane scene used by examples 2-4. Reuses the `CameraConfig`, `PendingInstance`, and `SceneConfig` patterns from `Advanced/SceneSetup.h` but creates geometry via the new `Primitive` library instead of loading OBJ files.

## Dependencies

- **Task 1** (Primitive Geometry Library): Required for `create_square()` and `create_cube()`.

## Implementation Steps

### 1. Create `examples/Common/SceneSetup.h`

**File:** `examples/Common/SceneSetup.h`

Provides the following inline functions and structs:

```cpp
#pragma once

#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Model/Material/ColoredMaterialHandler.h>
#include <VulkanWrapper/Model/Material/MaterialData.h>
#include <VulkanWrapper/Model/MeshManager.h>
#include <VulkanWrapper/Model/Primitive.h>
#include <VulkanWrapper/RayTracing/RayTracedScene.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>

struct CameraConfig {
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    [[nodiscard]] glm::mat4 view_matrix() const {
        return glm::lookAt(eye, target, up);
    }
};

struct UBOData {
    glm::mat4 proj = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 inverseViewProj = glm::mat4(1.0f);

    static UBOData create(float aspect_ratio, const glm::mat4 &view_matrix) {
        UBOData data;
        data.proj = glm::perspective(
            glm::radians(60.0f), aspect_ratio, 0.1f, 100.f);
        data.proj[1][1] *= -1; // Flip Y for Vulkan
        data.view = view_matrix;
        data.inverseViewProj = glm::inverse(data.proj * data.view);
        return data;
    }
};

struct PendingInstance {
    size_t mesh_index;
    glm::mat4 transform;
};

struct SceneConfig {
    CameraConfig camera;
    std::vector<PendingInstance> instances;
    size_t plane_mesh_index;
    size_t cube_mesh_index;
};
```

**Key inline functions:**

#### `add_colored_mesh`
Adds a `ModelPrimitiveResult` to the `MeshManager` with a solid color material:
```cpp
inline size_t add_colored_mesh(
    vw::Model::MeshManager &mesh_manager,
    vw::Model::ModelPrimitiveResult primitives,
    glm::vec3 color);
```
- Gets the `ColoredMaterialHandler` from `mesh_manager.material_manager().handler(colored_material_tag)`
- Casts to `ColoredMaterialHandler*` and calls `create_material(ColoredMaterialData{color})`
- Calls `mesh_manager.add_mesh(vertices, indices, material)`
- Returns the mesh index

#### `create_cube_plane_scene`
Creates the standard cube+plane scene:
```cpp
inline SceneConfig create_cube_plane_scene(
    vw::Model::MeshManager &mesh_manager);
```
- Creates ground plane: `create_square(PlanePrimitive::XZ)` with white color (`glm::vec3(0.8f)`)
- Ground plane instance transform: scale by 20x, so it spans `[-10, 10]` on XZ
- Creates cube: `create_cube()` with a color (e.g. `glm::vec3(0.7f, 0.3f, 0.3f)`)
- Cube instance transform: translate to `(0, 1.5, 0)`, so it floats above the ground plane
- Camera: positioned to see both cube and its shadow, e.g. `eye=(5, 4, 5)`, `target=(0, 0.5, 0)`
- Returns `SceneConfig` with indices and pending instances

#### `create_ubo`
Creates and fills a UBO buffer:
```cpp
inline auto create_ubo(
    const vw::Allocator &allocator,
    float aspect_ratio,
    const CameraConfig &camera);
```

#### `setup_ray_tracing`
Uploads meshes, adds instances, builds acceleration structures:
```cpp
inline void setup_ray_tracing(
    vw::Model::MeshManager &mesh_manager,
    vw::rt::RayTracedScene &ray_traced_scene,
    const vw::Device &device,
    const std::vector<PendingInstance> &instances);
```
- Calls `mesh_manager.fill_command_buffer()` and submits
- Adds instances to `ray_traced_scene`
- Sets material SBT mapping
- Calls `ray_traced_scene.build()`

### 2. Create `examples/Common/CMakeLists.txt`

This is a header-only component, so it just needs an INTERFACE library to provide the include path:

```cmake
add_library(ExamplesCommon INTERFACE)
target_include_directories(ExamplesCommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ExamplesCommon INTERFACE VulkanWrapper::VW)
```

Note: The INTERFACE include directory is set to `examples/` so consumers use `#include "Common/SceneSetup.h"`.

### 3. Register in `examples/CMakeLists.txt`

Add `add_subdirectory(Common)` before the example subdirectories:
```cmake
add_subdirectory(Common)
add_subdirectory(Application)
add_subdirectory(Advanced)
```

## Design Considerations

- **Near/far planes**: The primitives are unit-sized ([-0.5, 0.5]) and the scene uses transforms in the range ~[-10, 10], so near=0.1, far=100 is appropriate (different from the Advanced example's 2.0/5000.0 which works with Sponza scale).
- **SceneConfig stores mesh indices**: This allows examples like EmissiveCube to replace the cube's material by accessing `config.cube_mesh_index` before upload.
- **Separation of concerns**: `create_cube_plane_scene` only creates geometry and adds it to MeshManager with colored materials. Examples that need different materials (EmissiveCube) add the cube with a different call.

## Test Plan

No dedicated tests — this is utility code validated by the examples themselves compiling and producing correct screenshots.
