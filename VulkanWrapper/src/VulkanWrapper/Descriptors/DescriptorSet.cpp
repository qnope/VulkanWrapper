#include "VulkanWrapper/Descriptors/DescriptorSet.h"

namespace vw {

DescriptorSet::DescriptorSet(vk::DescriptorSet handle, std::vector<Barrier::ResourceState> resources) noexcept
    : ObjectWithHandle<vk::DescriptorSet>(handle)
    , m_resources(std::move(resources)) {}

const std::vector<Barrier::ResourceState>& DescriptorSet::resources() const noexcept {
    return m_resources;
}

} // namespace vw
