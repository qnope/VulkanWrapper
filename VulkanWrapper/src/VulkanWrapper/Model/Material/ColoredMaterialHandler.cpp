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
    std::shared_ptr<const Device> device, std::shared_ptr<Allocator> allocator)
    : Base{std::move(device), std::move(allocator)} {}

std::optional<ColoredMaterialData> ColoredMaterialHandler::try_create_gpu_data(
    const aiMaterial *mat, const std::filesystem::path & /*base_path*/) {
    aiColor4D diffuse_color(0.5f, 0.5f, 0.5f, 1.0f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color);

    return ColoredMaterialData{
        .color = glm::vec3(diffuse_color.r, diffuse_color.g, diffuse_color.b)};
}

} // namespace vw::Model::Material
