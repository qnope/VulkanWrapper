#pragma once

class aiMaterial;

namespace vw::Model::Internal {
struct MaterialInfo {
    MaterialInfo(const aiMaterial *material);

    std::optional<std::filesystem::path> diffuse_texture_path;
    std::optional<glm::vec4> diffuse_color;
};
} // namespace vw::Model::Internal
