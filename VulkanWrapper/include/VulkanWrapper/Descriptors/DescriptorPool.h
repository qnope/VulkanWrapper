#pragma once
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using DescriptorPoolCreationException =
    TaggedException<struct DescriptorPoolCreationTag>;

using DescriptorSetAllocationException =
    TaggedException<struct DescriptorSetAllocationTag>;

namespace Internal {
class DescriptorPoolImpl
    : public ObjectWithUniqueHandle<vk::UniqueDescriptorPool> {
  public:
    DescriptorPoolImpl(
        vk::UniqueDescriptorPool pool, const Device &device,
        const std::shared_ptr<const DescriptorSetLayout> &layout) noexcept;

    std::optional<vk::DescriptorSet> allocate_set();

  private:
    uint32_t m_number_allocation = 0;
    std::vector<vk::DescriptorSet> m_sets;
};
} // namespace Internal

class DescriptorPool {
  public:
    DescriptorPool(const Device &device,
                   std::shared_ptr<const DescriptorSetLayout> layout);

    vk::DescriptorSet
    allocate_set(const DescriptorAllocator &descriptorAllocator);

  private:
    vk::DescriptorSet allocate_descriptor_set_from_last_pool();

  private:
    const Device *m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
    std::vector<Internal::DescriptorPoolImpl> m_descriptor_pools;
    std::unordered_map<DescriptorAllocator, vk::DescriptorSet> m_sets;
};

class DescriptorPoolBuilder {
  public:
    DescriptorPoolBuilder(const Device &device,
                          const std::shared_ptr<DescriptorSetLayout> &layout);

    DescriptorPool build() &&;

  private:
    const Device *m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
};
} // namespace vw
