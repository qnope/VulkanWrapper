#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"

#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"

#include <algorithm>
#include <assimp/material.h>

namespace vw::Model::Material {

VW_DEFINE_MATERIAL_TYPE(textured_material_tag);

MaterialTypeTag TexturedMaterialHandler::tag() const {
    return textured_material_tag;
}

MaterialPriority TexturedMaterialHandler::priority() const {
    return textured_material_priority;
}

namespace {
// Convert Windows-style backslashes to forward slashes for cross-platform
// texture path compatibility
std::filesystem::path normalize_texture_path(const char *path) {
    std::string normalized{path};
    std::ranges::replace(normalized, '\\', '/');
    return std::filesystem::path{normalized};
}
} // namespace

std::optional<vk::DescriptorSet>
TexturedMaterialHandler::additional_descriptor_set() const {
    return m_texture_manager.descriptor_set();
}

std::shared_ptr<const DescriptorSetLayout>
TexturedMaterialHandler::additional_descriptor_set_layout() const {
    return m_texture_manager.layout();
}

TexturedMaterialHandler::TexturedMaterialHandler(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    BindlessTextureManager &texture_manager)
    : Base{std::move(device), std::move(allocator)}
    , m_texture_manager{texture_manager} {}

std::optional<TexturedMaterialData>
TexturedMaterialHandler::try_create_gpu_data(
    const aiMaterial *mat, const std::filesystem::path &base_path) {
    aiString texture_path;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) !=
        AI_SUCCESS) {
        return std::nullopt;
    }

    auto full_path = base_path / normalize_texture_path(texture_path.C_Str());
    if (!std::filesystem::exists(full_path)) {
        return std::nullopt;
    }

    uint32_t texture_index = m_texture_manager.register_texture(full_path);

    return TexturedMaterialData{.diffuse_texture_index = texture_index};
}

std::vector<Barrier::ResourceState>
TexturedMaterialHandler::get_texture_resources() const {
    return m_texture_manager.get_resources();
}

} // namespace vw::Model::Material
