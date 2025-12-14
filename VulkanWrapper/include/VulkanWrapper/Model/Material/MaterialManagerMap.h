#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/Material/MaterialManager.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include "VulkanWrapper/Utils/Error.h"

namespace vw::Model::Material {
class MaterialManagerMap {
  public:
    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout(MaterialTypeTag tag) const;

    template <const MaterialTypeTag *tag>
    ConcreteMaterialManager<tag> &manager() const {
        auto it = m_material_managers.find(*tag);
        if (it == m_material_managers.end()) {
            throw LogicException::invalid_state(
                "No material manager registered for material type");
        }
        return *static_cast<ConcreteMaterialManager<tag> *>(it->second.get());
    }

    template <const MaterialTypeTag *Tag>
    void insert_manager(std::unique_ptr<ConcreteMaterialManager<Tag>> manager) {
        m_material_managers.emplace(*Tag, std::move(manager));
    }

  private:
    std::unordered_map<MaterialTypeTag, std::unique_ptr<MaterialManager>>
        m_material_managers;
};
} // namespace vw::Model::Material
