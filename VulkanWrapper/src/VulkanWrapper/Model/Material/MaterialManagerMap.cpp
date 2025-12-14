#include "VulkanWrapper/Model/Material/MaterialManagerMap.h"

#include "VulkanWrapper/Model/Material/TexturedMaterialManager.h"
#include "VulkanWrapper/Utils/Error.h"

namespace vw::Model::Material {
std::shared_ptr<const DescriptorSetLayout>
MaterialManagerMap::layout(MaterialTypeTag tag) const {
    auto it = m_material_managers.find(tag);
    if (it == m_material_managers.end()) {
        throw LogicException::invalid_state(
            "No material manager registered for material type");
    }
    return it->second->layout();
}
} // namespace vw::Model::Material
