#include "VulkanWrapper/Model/Internal/MaterialInfo.h"

#include <assimp/material.h>

namespace vw::Model::Internal {
MaterialInfo::MaterialInfo(const aiMaterial *material,
                           const std::filesystem::path &directory_path) {
    decode_diffuse_texture(material, directory_path);
    decode_diffuse_color(material);
}

void MaterialInfo::decode_diffuse_texture(
    const aiMaterial *material, const std::filesystem::path &directory_path) {
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
    if (!path.empty()) {
        std::ranges::replace(path, '\\', '/');
        diffuse_texture_path = directory_path.string() + '/' + path;
    }
}

void MaterialInfo::decode_diffuse_color(const aiMaterial *material) {
    aiColor4D color;
    if (material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS) {
        diffuse_color.emplace(color.r, color.g, color.b, color.a);
    } else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) ==
               aiReturn_SUCCESS) {
        diffuse_color.emplace(color.r, color.g, color.b, color.a);
    }
}
} // namespace vw::Model::Internal
