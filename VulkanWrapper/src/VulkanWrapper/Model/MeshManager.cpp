#include "VulkanWrapper/Model/MeshManager.h"

namespace vw {
namespace {
std::shared_ptr<DescriptorSetLayout> create_layout(const Device &device) {
    return DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();
}

DescriptorPool create_pool(const Device &device,
                           std::shared_ptr<DescriptorSetLayout> layout) {
    return DescriptorPoolBuilder(device, std::move(layout), 1'000).build();
}
} // namespace

MeshManager::MeshManager(const Device &device, const Allocator &allocator)
    : m_staging_buffer_manager{device, allocator}
    , m_descriptor_set_layout{create_layout(device)}
    , m_descriptor_pool{create_pool(device, m_descriptor_set_layout)} {}
} // namespace vw
