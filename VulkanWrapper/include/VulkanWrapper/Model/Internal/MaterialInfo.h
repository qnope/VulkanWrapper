#pragma once

class aiMaterial;

namespace vw::Model::Internal {
struct MaterialInfo {
    MaterialInfo(const aiMaterial *material,
                 const std::filesystem::path &directory_path);

    std::optional<std::filesystem::path> diffuse_texture_path;
    std::optional<glm::vec4> diffuse_color;

  private:
    void decode_diffuse_texture(const aiMaterial *material,
                                const std::filesystem::path &directory_path);
    void decode_diffuse_color(const aiMaterial *material);
};
} // namespace vw::Model::Internal
