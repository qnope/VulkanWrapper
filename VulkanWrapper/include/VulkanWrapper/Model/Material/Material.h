#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {
struct Material {
    const MaterialTypeTag *material_type;
    DescriptorSet descriptor_set;
};
} // namespace vw::Model::Material
