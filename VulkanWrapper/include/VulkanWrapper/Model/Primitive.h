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

/// Equilateral triangle centered at origin, unit size
/// (fits in [-0.5, 0.5]).
ModelPrimitiveResult create_triangle(PlanePrimitive plane);

/// Unit square centered at origin. 4 vertices + 6 indices
/// (2 triangles).
ModelPrimitiveResult create_square(PlanePrimitive plane);

/// Unit cube centered at origin, fits in [-0.5, 0.5].
/// 24 vertices (4 per face) + 36 indices with correct
/// normals/tangents/UVs.
ModelPrimitiveResult create_cube();

} // namespace vw::Model
