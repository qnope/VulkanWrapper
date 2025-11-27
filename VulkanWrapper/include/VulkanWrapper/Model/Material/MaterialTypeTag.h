#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Utils/IdentifierTag.h"

namespace vw::Model::Material {
using MaterialTypeTag = IdentifierTag<struct IdentifierMaterialTag>;

template <typename MaterialTag>
constexpr MaterialTypeTag create_material_type_tag() {
    return MaterialTypeTag(typeid(MaterialTag));
}

} // namespace vw::Model::Material
