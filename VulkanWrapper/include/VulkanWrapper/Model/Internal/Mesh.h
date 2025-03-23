#pragma once

#include "VulkanWrapper/Descriptors/Vertex.h"

struct aiMesh;

namespace vw::Model::Internal {
struct Mesh {
    Mesh(const aiMesh *mesh);

    std::vector<FullVertex3D> vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index;
};
} // namespace vw::Model::Internal
