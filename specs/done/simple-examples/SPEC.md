# Simple Examples - Specification

## 1. Feature Overview

Implement 4 standalone examples that showcase the VulkanWrapper library's capabilities at increasing complexity levels. Each example produces an automatic screenshot and lives in its own directory under `examples/`.

A new `Primitive.h` / `Primitive.cpp` module is added to the library (`Model/`) to provide reusable geometry generators.

### Examples Summary

| # | Name | Description | Rendering |
|---|------|-------------|-----------|
| 1 | **Triangle** | Colored triangle | Direct minimal (no deferred pipeline) |
| 2 | **CubeShadow** | Cube floating above a plane, sunlit with shadow | Full deferred + ray tracing |
| 3 | **AmbientOcclusion** | Same scene as 2, raw AO displayed in grayscale | Full deferred + ray tracing (AO only) |
| 4 | **EmissiveCube** | Same scene as 2, cube uses emissive textured material | Full deferred + ray tracing |

---

## 2. API Design

### 2.1 Primitive Geometry Library (`Model/Primitive.h`)

New library files:
- `VulkanWrapper/include/VulkanWrapper/Model/Primitive.h`
- `VulkanWrapper/src/VulkanWrapper/Model/Primitive.cpp`

Registered in the corresponding `CMakeLists.txt` files (PUBLIC header, PRIVATE source).

```cpp
namespace vw::Model {

enum class PlanePrimitive {
    XY, // parallel to Z
    XZ, // parallel to Y
    YZ // parallel to X
};

struct ModelPrimitiveResult
{
    std::vector<FullVertex3D> vertices;
    std::vector<uint32_t> indices;
}

/// Equilateral triangle centered at origin, unit size (fits in [-0.5, 0.5]).
/// Vertices have correct normals, tangents, bitangents, and UVs.
ModelPrimitiveResult create_triangle(PlanePrimitive);

/// Unit square centered at origin in the given Plane.
/// Fits in [-0.5, 0.5]. Two triangles (4 vertices + indices).
ModelPrimitiveResult create_square(PlanePrimitive);

/// Unit cube centered at origin, fits in [-0.5, 0.5].
/// Each face has independent vertices with correct normals, tangents, bitangents, and UVs.
ModelPrimitiveResult create_cube();

} // namespace vw::Model
```

All primitives:
- Centered at origin
- Unit size (fit within [-0.5, 0.5] on all axes)
- Correct normals, tangents, bitangents for lighting
- UV coordinates in [0, 1]
- Consumer scales/translates via transform matrix

### 2.2 Example 1: Triangle (`examples/Triangle/`)

**Goal:** Minimal direct rendering of a colored triangle. Demonstrates core VulkanWrapper usage without the deferred pipeline.

**User Stories:**
- As a new user, I can see how to set up Instance, Device, Swapchain, and a simple graphics pipeline.
- As a new user, I can see how push constants are used to pass a color to the fragment shader.

**Behavior:**
- Uses `App` class from `examples/Application/` for Vulkan setup.
- Creates geometry from `Primitive::create_triangle()`.
- Uploads vertex + index buffers.
- Custom vertex shader: transforms `FullVertex3D` positions with a model-view-projection push constant.
- Custom fragment shader: outputs a solid color from a push constant (`vec3 color`).
- Renders directly with `cmd.beginRendering()` / `cmd.endRendering()` (no RenderPipeline).
- Produces `screenshot.png` after a few frames.

**Shaders:**
- `examples/Triangle/triangle.vert` - Simple MVP transform.
- `examples/Triangle/triangle.frag` - Outputs push constant color.

**Push constants struct:**
```cpp
struct TrianglePushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};
```

### 2.3 Example 2: CubeShadow (`examples/CubeShadow/`)

**Goal:** A cube floating above a ground plane, lit by the sun with visible shadow on the ground.

**User Stories:**
- As a user, I can see how to set up a complete deferred rendering pipeline with ray tracing.
- As a user, I can see how to create a scene programmatically using Primitive functions and the MeshManager.
- As a user, I can see how the Slot-based RenderPipeline wires passes together.

**Behavior:**
- Uses `App` class for Vulkan setup.
- Scene: ground plane (`create_square()` scaled large) + cube (`create_cube()` translated above the plane).
- Both meshes use `ColoredMaterialHandler` (e.g., white plane, colored cube).
- Full deferred pipeline: `ZPass -> DirectLightPass -> SkyPass -> ToneMappingPass`.
- Sun direction configured to cast a visible shadow of the cube onto the plane.
- Camera positioned to see both the cube and its shadow.
- Produces `screenshot.png` after sufficient frames.

**Render Pipeline:**
```
ZPass -> DirectLightPass -> SkyPass -> ToneMappingPass
```

### 2.4 Example 3: AmbientOcclusion (`examples/AmbientOcclusion/`)

**Goal:** Same cube+plane scene, but the final output displays the raw ambient occlusion buffer in grayscale.

**User Stories:**
- As a user, I can see how to configure the RenderPipeline to visualize intermediate buffers.
- As a user, I can see how AO is computed via ray queries.

**Behavior:**
- Same scene setup as Example 2 (cube + plane, same camera, same materials).
- Pipeline: `ZPass -> DirectLightPass -> AmbientOcclusionPass`.
- Progressive accumulation: screenshot taken after enough samples for clean AO.
- Produces `screenshot.png`.

**Render Pipeline:**
```
ZPass -> DirectLightPass -> AmbientOcclusionPass -> [AO display pass] -> Swapchain
```

### 2.5 Example 4: EmissiveCube (`examples/EmissiveCube/`)

**Goal:** Same cube+plane scene, but the cube uses an emissive textured material (stained.png).

**User Stories:**
- As a user, I can see how to use EmissiveTexturedMaterialHandler for light-emitting objects.
- As a user, I can see how emissive materials affect indirect lighting in the scene.

**Behavior:**
- Same scene layout as Example 2 (cube + plane, same camera).
- The plane uses `ColoredMaterialHandler` (white).
- The cube uses `EmissiveTexturedMaterialHandler` with `Images/stained.png` and a high intensity.
- Full deferred pipeline with indirect lighting: `ZPass -> DirectLightPass -> AmbientOcclusionPass -> SkyPass -> IndirectLightPass -> ToneMappingPass`.
- The emissive cube should visibly illuminate the surrounding ground plane via indirect light.
- Progressive accumulation for indirect lighting.
- Produces `screenshot.png` after sufficient samples.

**Render Pipeline:**
```
ZPass -> DirectLightPass -> AmbientOcclusionPass -> SkyPass -> IndirectLightPass -> ToneMappingPass
```

---

## 3. Directory Structure

```
examples/
├── Application/          # Existing - shared App class
├── Advanced/             # Existing - Sponza deferred rendering
├── Triangle/             # NEW - Example 1
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── triangle.vert
│   └── triangle.frag
├── CubeShadow/           # NEW - Example 2
│   ├── CMakeLists.txt
│   └── main.cpp
├── AmbientOcclusion/     # NEW - Example 3
│   ├── CMakeLists.txt
│   └── main.cpp
└── EmissiveCube/         # NEW - Example 4
    ├── CMakeLists.txt
    └── main.cpp

VulkanWrapper/
├── include/VulkanWrapper/Model/
│   └── Primitive.h       # NEW - Geometry generators
└── src/VulkanWrapper/Model/
    └── Primitive.cpp      # NEW - Geometry generators
```

---

## 4. Testing and Validation

### Automated Tests
- **Primitive unit tests** (`VulkanWrapper/tests/Model/PrimitiveTests.cpp`):
  - `create_triangle()` returns 3 vertices with correct normals and UVs.
  - `create_square()` returns 4 vertices, `create_square_indices()` returns 6 indices.
  - `create_cube()` returns 24 vertices (4 per face), `create_cube_indices()` returns 36 indices.
  - All primitives fit within [-0.5, 0.5] bounding box.
  - Normals are unit length.
  - Tangents are orthogonal to normals.

### Visual Validation
- Each example produces a `screenshot.png` that can be visually inspected.
- Example 1: A single colored triangle on a black background.
- Example 2: A lit cube with a visible shadow on a ground plane, sky in the background.
- Example 3: Grayscale AO visualization showing occlusion at the cube-plane contact area.
- Example 4: An emissive cube illuminating the surrounding plane with colored indirect light.

### Build Validation
- All 4 examples compile and link successfully.
- Each example can be run independently: `cd build/examples/<Name> && DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib ./<Name>`.
