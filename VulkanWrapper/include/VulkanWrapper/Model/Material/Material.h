#pragma once

#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {
struct Material {
    MaterialTypeTag material_type;
    vk::DescriptorSet descriptor_set;
};
} // namespace vw::Model::Material
