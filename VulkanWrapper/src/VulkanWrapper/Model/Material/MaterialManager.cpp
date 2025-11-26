#include "VulkanWrapper/Model/Material/MaterialManager.h"

namespace vw::Model::Material {

MaterialManager::MaterialManager(DescriptorPool pool)
    : m_descriptor_pool{std::move(pool)} {}

std::shared_ptr<const DescriptorSetLayout>
MaterialManager::layout() const noexcept {
    return m_descriptor_pool.layout();
}

DescriptorSet
MaterialManager::allocate_set(const DescriptorAllocator &allocator) {
    return m_descriptor_pool.allocate_set(allocator);
}
} // namespace vw::Model::Material
