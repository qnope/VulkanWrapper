#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Model/Material/MaterialManager.h"
#include "VulkanWrapper/Model/Material/MaterialManagerMap.h"
#include "VulkanWrapper/Model/Material/MaterialPriority.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {

class MaterialFactory {
    using Function = std::function<std::optional<Material>(
        const Internal::MaterialInfo &info)>;

  public:
    MaterialFactory(std::shared_ptr<MaterialManagerMap> material_manager_map) noexcept;
    [[nodiscard]] Material
    allocate_material(const Internal::MaterialInfo &) const noexcept;

    template <const MaterialTypeTag *tag>
    void insert_factory(
        MaterialPriority priority,
        std::function<std::optional<Material>(const Internal::MaterialInfo &,
                                              ConcreteMaterialManager<tag> &)>
            concrete_factory) {
        Function f = [concrete_factory = std::move(concrete_factory),
                      this](const Internal::MaterialInfo &info) {
            auto &manager = m_material_manager_map->manager<tag>();
            return concrete_factory(info, manager);
        };
        m_factories.emplace(priority, std::move(f));
    }

  private:
    std::shared_ptr<MaterialManagerMap> m_material_manager_map;
    std::map<MaterialPriority, Function, std::greater<>> m_factories;
};
} // namespace vw::Model::Material
