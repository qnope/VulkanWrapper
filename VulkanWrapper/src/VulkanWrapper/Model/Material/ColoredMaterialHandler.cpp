#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"

#include <assimp/material.h>

namespace vw::Model::Material {

VW_DEFINE_MATERIAL_TYPE(colored_material_tag);

MaterialTypeTag ColoredMaterialHandler::tag() const {
    return colored_material_tag;
}

MaterialPriority ColoredMaterialHandler::priority() const {
    return colored_material_priority;
}

ColoredMaterialHandler::ColoredMaterialHandler(
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator,
    BindlessTextureManager &texture_manager)
    : Base{std::move(device), std::move(allocator), texture_manager} {}

std::optional<ColoredMaterialData> ColoredMaterialHandler::try_create_gpu_data(
    const aiMaterial *mat, const std::filesystem::path & /*base_path*/) {
    aiColor4D diffuse_color(0.5f, 0.5f, 0.5f, 1.0f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);

    return ColoredMaterialData{
        .color = glm::vec4(diffuse_color.r, diffuse_color.g, diffuse_color.b,
                           diffuse_color.a)};
}

void ColoredMaterialHandler::setup_layout(DescriptorSetLayoutBuilder &builder) {
    builder.with_storage_buffer(vk::ShaderStageFlagBits::eFragment);
}

void ColoredMaterialHandler::setup_descriptors(DescriptorAllocator &alloc) {
    // binding 0 = SSBO
    add_ssbo_descriptor(alloc, 0);
}

} // namespace vw::Model::Material
