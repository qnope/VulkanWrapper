#pragma once

#include "VulkanWrapper/Model/Material/Material.h"
#include "VulkanWrapper/Model/Material/MaterialManager.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"

namespace vw::Model::Material {
class MaterialManagerMap {
  public:
    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout(MaterialTypeTag tag) const noexcept;

    template <const MaterialTypeTag *tag, typename... Args>
    Material allocate_material(Args &&...args) {
        auto it = m_material_managers.find(*tag);
        assert(it != m_material_managers.end());
        return static_cast<ConcreteMaterialManager<tag> *>(it->second.get())
            ->allocate(std::forward<Args>(args)...);
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
