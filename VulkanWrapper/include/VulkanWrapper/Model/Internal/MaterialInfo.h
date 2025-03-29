#pragma once

class aiMaterial;

namespace vw::Model::Internal {
struct MaterialInfo {
    MaterialInfo(const aiMaterial *material);

    std::filesystem::path diffuse_texture_path;
};
} // namespace vw::Model::Internal
