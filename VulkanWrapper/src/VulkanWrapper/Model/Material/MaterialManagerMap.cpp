#include "VulkanWrapper/Model/Material/MaterialManagerMap.h"

#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"

namespace vw::Model::Material {
std::shared_ptr<const DescriptorSetLayout>
MaterialManagerMap::layout(MaterialTypeTag tag) const noexcept {
    auto it = m_material_managers.find(tag);
    assert(it != m_material_managers.end());
    return it->second->layout();
}
} // namespace vw::Model::Material
