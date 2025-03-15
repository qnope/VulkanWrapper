#include "VulkanWrapper/Descriptors/DescriptorPool.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

DescriptorPool::DescriptorPool(
    const Device &device, std::shared_ptr<const DescriptorSetLayout> layout,
    vk::UniqueDescriptorPool pool)
    : ObjectWithUniqueHandle<vk::UniqueDescriptorPool>{std::move(pool)}
    , m_device{device}
    , m_layout{std::move(layout)} {}

vk::DescriptorSet
DescriptorPool::allocate_set(const DescriptorAllocator &descriptorAllocator) {
    auto it = m_sets.find(descriptorAllocator);
    if (it != m_sets.end())
        return it->second;

    auto layout_handle = m_layout->handle();
    auto info = vk::DescriptorSetAllocateInfo()
                    .setDescriptorPool(handle())
                    .setSetLayouts(layout_handle);
    auto [result, value] = m_device.handle().allocateDescriptorSets(info);
    if (result != vk::Result::eSuccess)
        throw DescriptorSetAllocationException{std::source_location::current()};

    auto writers = descriptorAllocator.get_write_descriptors();
    for (auto &writer : writers)
        writer.setDstSet(value.front());

    m_device.handle().updateDescriptorSets(writers, nullptr);

    m_sets.emplace(descriptorAllocator, value.front());

    return value.front();
}

DescriptorPoolBuilder::DescriptorPoolBuilder(
    const Device &device, std::shared_ptr<DescriptorSetLayout> layout,
    int number_of_set)
    : m_device{device}
    , m_number_of_set{number_of_set}
    , m_layout{layout} {}

DescriptorPool DescriptorPoolBuilder::build() && {
    const auto pool_size = m_layout->get_pool_sizes();
    const auto info = vk::DescriptorPoolCreateInfo()
                          .setMaxSets(m_number_of_set)
                          .setPoolSizes(pool_size);
    auto [result, value] = m_device.handle().createDescriptorPoolUnique(info);

    if (result != vk::Result::eSuccess)
        throw DescriptorPoolCreationException(std::source_location::current());
    return DescriptorPool{m_device, std::move(m_layout), std::move(value)};
}

} // namespace vw
