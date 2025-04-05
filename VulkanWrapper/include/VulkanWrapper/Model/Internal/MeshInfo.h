#pragma once

#include "VulkanWrapper/Descriptors/Vertex.h"

struct aiMesh;

namespace vw::Model::Internal {
struct MeshInfo {
    MeshInfo(const aiMesh *mesh);

    std::vector<Vertex3D> vertices;
    std::vector<FullVertex3D> full_vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index;
};
} // namespace vw::Model::Internal
