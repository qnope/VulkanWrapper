#include "VulkanWrapper/Model/Material/MaterialFactory.h"

#include <exception>

namespace vw::Model::Material {
MaterialFactory::MaterialFactory(
    MaterialManagerMap &material_manager_map) noexcept
    : m_material_manager_map{&material_manager_map} {}

Material MaterialFactory::allocate_material(
    const Internal::MaterialInfo &info) const noexcept {
    for (const auto &factory : m_factories | std::views::values)
        if (auto material = factory(info))
            return *material;
    std::terminate();
}
} // namespace vw::Model::Material
