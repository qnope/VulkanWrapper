#include "VulkanWrapper/Model/Material/EmissiveTexturedMaterialHandler.h"

#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"

#include <algorithm>
#include <assimp/material.h>

namespace vw::Model::Material {

VW_DEFINE_MATERIAL_TYPE(emissive_textured_material_tag);

MaterialTypeTag EmissiveTexturedMaterialHandler::tag() const {
    return emissive_textured_material_tag;
}

MaterialPriority EmissiveTexturedMaterialHandler::priority() const {
    return emissive_textured_material_priority;
}

namespace {
std::filesystem::path normalize_texture_path(const char *path) {
    std::string normalized{path};
    std::ranges::replace(normalized, '\\', '/');
    return std::filesystem::path{normalized};
}
} // namespace

std::optional<vk::DescriptorSet>
EmissiveTexturedMaterialHandler::additional_descriptor_set() const {
    return m_texture_manager.descriptor_set();
}

std::shared_ptr<const DescriptorSetLayout>
EmissiveTexturedMaterialHandler::additional_descriptor_set_layout()
    const {
    return m_texture_manager.layout();
}

EmissiveTexturedMaterialHandler::EmissiveTexturedMaterialHandler(
    std::shared_ptr<const Device> device,
    std::shared_ptr<Allocator> allocator,
    BindlessTextureManager &texture_manager)
    : Base{std::move(device), std::move(allocator)}
    , m_texture_manager{texture_manager} {}

Material EmissiveTexturedMaterialHandler::create_material(
    const std::filesystem::path &texture_path, float intensity) {
    uint32_t texture_index =
        m_texture_manager.register_texture(texture_path);
    return Base::create_material(EmissiveTexturedMaterialData{
        .diffuse_texture_index = texture_index,
        .intensity = intensity});
}

std::optional<EmissiveTexturedMaterialData>
EmissiveTexturedMaterialHandler::try_create_gpu_data(
    const aiMaterial *mat, const std::filesystem::path &base_path) {
    aiString texture_path;
    if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &texture_path) !=
        AI_SUCCESS) {
        return std::nullopt;
    }

    auto full_path =
        base_path / normalize_texture_path(texture_path.C_Str());
    if (!std::filesystem::exists(full_path)) {
        return std::nullopt;
    }

    uint32_t texture_index =
        m_texture_manager.register_texture(full_path);

    float intensity = 1.0f;
    mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, intensity);

    return EmissiveTexturedMaterialData{
        .diffuse_texture_index = texture_index,
        .intensity = intensity};
}

std::vector<Barrier::ResourceState>
EmissiveTexturedMaterialHandler::get_texture_resources() const {
    return m_texture_manager.get_resources();
}

} // namespace vw::Model::Material
