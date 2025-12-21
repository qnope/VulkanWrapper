#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"

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

TexturedMaterialHandler::TexturedMaterialHandler(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    BindlessTextureManager &texture_manager)
    : Base{std::move(device), std::move(allocator), texture_manager} {}

std::optional<TexturedMaterialData> TexturedMaterialHandler::try_create_gpu_data(
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

    uint32_t texture_index = texture_manager().register_texture(full_path);

    return TexturedMaterialData{.diffuse_texture_index = texture_index,
                                ._pad = {0, 0, 0}};
}

void TexturedMaterialHandler::setup_layout(DescriptorSetLayoutBuilder &builder) {
    // Layout matches shader: binding 0 = sampler, binding 1 = SSBO, binding 2 =
    // textures
    builder.with_sampler(vk::ShaderStageFlagBits::eFragment)
        .with_storage_buffer(vk::ShaderStageFlagBits::eFragment)
        .with_sampled_images_bindless(vk::ShaderStageFlagBits::eFragment,
                                      BindlessTextureManager::MAX_TEXTURES);
}

void TexturedMaterialHandler::setup_descriptors(DescriptorAllocator &alloc) {
    // binding 0 = sampler, binding 1 = SSBO, binding 2 = textures
    alloc.add_sampler(0, texture_manager().sampler());
    add_ssbo_descriptor(alloc, 1);
    texture_manager().write_image_descriptors(descriptor_set(), 2);
}

std::vector<Barrier::ResourceState>
TexturedMaterialHandler::get_texture_resources() const {
    return texture_manager().get_resources();
}

} // namespace vw::Model::Material
