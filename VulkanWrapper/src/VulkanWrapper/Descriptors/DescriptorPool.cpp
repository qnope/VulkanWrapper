#include "VulkanWrapper/Descriptors/DescriptorPool.h"

#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

constexpr auto MAX_DESCRIPTOR_SET_BY_POOL = 16;
namespace Internal {
DescriptorPoolImpl::DescriptorPoolImpl(
    vk::UniqueDescriptorPool pool, const Device &device,
    const std::shared_ptr<const DescriptorSetLayout> &layout)
    : ObjectWithUniqueHandle<vk::UniqueDescriptorPool>{std::move(pool)} {
    std::vector layouts(MAX_DESCRIPTOR_SET_BY_POOL, layout->handle());

    const auto info = vk::DescriptorSetAllocateInfo()
                          .setDescriptorPool(handle())
                          .setSetLayouts(layouts);

    auto [result, sets] = device.handle().allocateDescriptorSets(info);
    if (result != vk::Result::eSuccess) {
        throw DescriptorSetAllocationException{std::source_location::current()};
    }
    m_sets = std::move(sets);
}

std::optional<vk::DescriptorSet> DescriptorPoolImpl::allocate_set() {
    if (m_number_allocation < m_sets.size())
        return m_sets[m_number_allocation++];
    return std::nullopt;
}
} // namespace Internal
DescriptorPool::DescriptorPool(
    const Device &device, std::shared_ptr<const DescriptorSetLayout> layout)
    : m_device{&device}
    , m_layout{std::move(layout)} {}

std::shared_ptr<const DescriptorSetLayout>
DescriptorPool::layout() const noexcept {
    return m_layout;
}

vk::DescriptorSet DescriptorPool::allocate_set(
    const DescriptorAllocator &descriptorAllocator) noexcept {
    auto it = m_sets.find(descriptorAllocator);
    if (it != m_sets.end()) {
        return it->second;
    }

    auto set = allocate_descriptor_set_from_last_pool();

    auto writers = descriptorAllocator.get_write_descriptors();
    for (auto &writer : writers) {
        writer.setDstSet(set);
    }

    m_device->handle().updateDescriptorSets(writers, nullptr);
    m_sets.emplace(descriptorAllocator, set);

    return set;
}

vk::DescriptorSet DescriptorPool::allocate_descriptor_set_from_last_pool() {
    if (!m_descriptor_pools.empty()) {
        if (auto set = m_descriptor_pools.back().allocate_set())
            return *set;
    }

    auto pool_sizes = m_layout->get_pool_sizes();
    for (auto &pool_size : pool_sizes)
        pool_size.descriptorCount *= MAX_DESCRIPTOR_SET_BY_POOL;

    const auto info = vk::DescriptorPoolCreateInfo()
                          .setMaxSets(MAX_DESCRIPTOR_SET_BY_POOL)
                          .setPoolSizes(pool_sizes);

    auto [result, pool] = m_device->handle().createDescriptorPoolUnique(info);
    if (result != vk::Result::eSuccess) {
        throw DescriptorPoolCreationException(std::source_location::current());
    }

    auto set =
        m_descriptor_pools.emplace_back(std::move(pool), *m_device, m_layout)
            .allocate_set();
    assert(set);
    return *set;
}

DescriptorPoolBuilder::DescriptorPoolBuilder(
    const Device &device, const std::shared_ptr<DescriptorSetLayout> &layout)
    : m_device{&device}
    , m_layout{layout} {}

DescriptorPool DescriptorPoolBuilder::build() && {

    return DescriptorPool{*m_device, std::move(m_layout)};
}

} // namespace vw
