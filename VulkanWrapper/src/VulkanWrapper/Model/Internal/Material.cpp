#include "VulkanWrapper/Model/Internal/Material.h"

#include <assimp/material.h>

namespace vw::Model::Internal {
Material::Material(const aiMaterial *material) {
    std::string path;
    if (material->GetTextureCount(aiTextureType::aiTextureType_BASE_COLOR) >
        0) {
        aiString string;
        material->GetTexture(aiTextureType::aiTextureType_BASE_COLOR, 0,
                             &string);
        path = string.C_Str();
    } else if (material->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE) >
               0) {
        aiString string;
        material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &string);
        path = string.C_Str();
    }
    std::ranges::replace(path, '\\', '/');
    diffuse_texture_path = path;
}
} // namespace vw::Model::Internal
