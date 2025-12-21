#pragma once
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include <cstdint>

namespace vw::Model::Material {

struct Material {
    MaterialTypeTag material_type;
    uint32_t material_index;
};

} // namespace vw::Model::Material
