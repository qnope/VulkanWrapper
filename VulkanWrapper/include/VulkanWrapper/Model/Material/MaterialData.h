#pragma once
#include "VulkanWrapper/3rd_party.h"
#include <cstdint>

namespace vw::Model::Material {

struct TexturedMaterialData {
    uint32_t diffuse_texture_index;
    uint32_t _pad[3];
};
static_assert(sizeof(TexturedMaterialData) == 16);

struct ColoredMaterialData {
    glm::vec4 color;
};
static_assert(sizeof(ColoredMaterialData) == 16);

} // namespace vw::Model::Material
