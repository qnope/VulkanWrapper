#pragma once
#include "VulkanWrapper/3rd_party.h"
#include <cstdint>

namespace vw::Model::Material {

struct TexturedMaterialData {
    uint32_t diffuse_texture_index;
};
static_assert(sizeof(TexturedMaterialData) == 4);

struct ColoredMaterialData {
    glm::vec3 color;
};
static_assert(sizeof(ColoredMaterialData) == 12);

} // namespace vw::Model::Material
