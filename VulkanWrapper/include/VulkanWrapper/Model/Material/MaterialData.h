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

struct EmissiveTexturedMaterialData {
    uint32_t diffuse_texture_index;
    float intensity; // lumen / m2
};
static_assert(sizeof(EmissiveTexturedMaterialData) == 8);

} // namespace vw::Model::Material
