# Task 1: Primitive Geometry Library

## Summary

Add `Primitive.h` / `Primitive.cpp` to the library's `Model/` module, providing `create_triangle()`, `create_square()`, and `create_cube()` functions that return `ModelPrimitiveResult` (vertices + indices). Unit tests validate geometry correctness.

## Dependencies

- None (this is the foundation task).

## Implementation Steps

### 1. Create `Primitive.h`

**File:** `VulkanWrapper/include/VulkanWrapper/Model/Primitive.h`

```cpp
#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/Vertex.h"

namespace vw::Model {

enum class PlanePrimitive {
    XY, // parallel to Z
    XZ, // parallel to Y
    YZ  // parallel to X
};

struct ModelPrimitiveResult {
    std::vector<FullVertex3D> vertices;
    std::vector<uint32_t> indices;
};

/// Equilateral triangle centered at origin, unit size (fits in [-0.5, 0.5]).
ModelPrimitiveResult create_triangle(PlanePrimitive plane);

/// Unit square centered at origin. 4 vertices + 6 indices (2 triangles).
ModelPrimitiveResult create_square(PlanePrimitive plane);

/// Unit cube centered at origin, fits in [-0.5, 0.5].
/// 24 vertices (4 per face) + 36 indices with correct normals/tangents/UVs.
ModelPrimitiveResult create_cube();

} // namespace vw::Model
```

### 2. Create `Primitive.cpp`

**File:** `VulkanWrapper/src/VulkanWrapper/Model/Primitive.cpp`

Key implementation details:

- **`create_triangle(PlanePrimitive)`**: 3 vertices forming an equilateral triangle centered at origin. Choose two axes from `{X, Y, Z}` based on the `PlanePrimitive` enum. The third axis defines the normal direction.
  - Vertices at 120-degree intervals, radius = `1 / sqrt(3)` to fit in `[-0.5, 0.5]`.
  - Normal = unit vector along the perpendicular axis.
  - Tangent = unit vector along the first chosen axis.
  - Bitangent = cross(normal, tangent).
  - UVs: map from vertex positions to `[0, 1]`.
  - Indices: `{0, 1, 2}`.

- **`create_square(PlanePrimitive)`**: 4 vertices at corners `(-0.5, -0.5)`, `(0.5, -0.5)`, `(0.5, 0.5)`, `(-0.5, 0.5)` in the chosen plane.
  - Normal, tangent, bitangent same logic as triangle.
  - UVs: `(0,0)`, `(1,0)`, `(1,1)`, `(0,1)`.
  - Indices: `{0, 1, 2, 0, 2, 3}`.

- **`create_cube()`**: 6 faces, each a quad with 4 independent vertices.
  - Each face has its own normal pointing outward (`+X, -X, +Y, -Y, +Z, -Z`).
  - Tangent/bitangent per face (consistent with the face's UV mapping).
  - UVs: `(0,0)` to `(1,1)` per face.
  - 24 vertices total, 36 indices total (6 indices per face).
  - Winding order: counter-clockwise when viewed from outside.

**Axis mapping for `PlanePrimitive`:**

| Enum | Axis 1 (U) | Axis 2 (V) | Normal axis |
|------|-----------|-----------|-------------|
| `XY` | X | Y | +Z |
| `XZ` | X | Z | +Y |
| `YZ` | Y | Z | +X |

### 3. Register in CMake

**`VulkanWrapper/include/VulkanWrapper/Model/CMakeLists.txt`** — add `Primitive.h` to PUBLIC sources:
```cmake
target_sources(VulkanWrapperCoreLibrary PUBLIC
    Importer.h
    Mesh.h
    MeshManager.h
    Primitive.h
)
```

**`VulkanWrapper/src/VulkanWrapper/Model/CMakeLists.txt`** — add `Primitive.cpp` to PRIVATE sources:
```cmake
target_sources(VulkanWrapperCoreLibrary PRIVATE
    Importer.cpp
    Mesh.cpp
    MeshManager.cpp
    Primitive.cpp
)
```

### 4. Unit Tests

**File:** `VulkanWrapper/tests/Model/PrimitiveTests.cpp`

**Register in `VulkanWrapper/tests/CMakeLists.txt`** — add a new test executable:

```cmake
add_executable(PrimitiveTests
    Model/PrimitiveTests.cpp
)

target_link_libraries(PrimitiveTests
    PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(PrimitiveTests)
```

**Test cases:**

| Test | What it checks |
|------|---------------|
| `Triangle_Returns3Vertices` | `create_triangle(XZ).vertices.size() == 3` |
| `Triangle_Returns3Indices` | `create_triangle(XZ).indices.size() == 3` |
| `Triangle_FitsInBoundingBox` | All vertex positions within `[-0.5, 0.5]` |
| `Triangle_NormalsAreUnitLength` | `glm::length(normal) ≈ 1.0` for each vertex |
| `Triangle_TangentsOrthogonalToNormals` | `dot(tangent, normal) ≈ 0.0` for each vertex |
| `Triangle_UVsInRange` | UVs in `[0, 1]` |
| `Triangle_AllPlanes` | Run the above for each `PlanePrimitive` value |
| `Square_Returns4Vertices` | `create_square(XZ).vertices.size() == 4` |
| `Square_Returns6Indices` | `create_square(XZ).indices.size() == 6` |
| `Square_FitsInBoundingBox` | All vertex positions within `[-0.5, 0.5]` |
| `Square_NormalsAreUnitLength` | All normals unit length |
| `Square_TangentsOrthogonalToNormals` | Orthogonality check |
| `Square_UVsCoverFullRange` | UVs span from `(0,0)` to `(1,1)` |
| `Square_AllPlanes` | Run for each `PlanePrimitive` |
| `Cube_Returns24Vertices` | 4 vertices per face x 6 faces |
| `Cube_Returns36Indices` | 6 indices per face x 6 faces |
| `Cube_FitsInBoundingBox` | All positions within `[-0.5, 0.5]` |
| `Cube_NormalsAreUnitLength` | All normals unit length |
| `Cube_TangentsOrthogonalToNormals` | Orthogonality check |
| `Cube_EachFaceHasConsistentNormal` | Each group of 4 vertices shares the same normal |
| `Cube_AllSixNormalDirections` | The 6 face normals cover `{+X, -X, +Y, -Y, +Z, -Z}` |

## Special Notes

- These tests do NOT require a GPU — they are pure CPU geometry tests. The `TestUtils` link provides `FullVertex3D` via the library headers.
- Use `EXPECT_NEAR` with epsilon `1e-5f` for floating-point comparisons.
- The `create_cube()` function does not take a `PlanePrimitive` parameter since a cube uses all three axes.
