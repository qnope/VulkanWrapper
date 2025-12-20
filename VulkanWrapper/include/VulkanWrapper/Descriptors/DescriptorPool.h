#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Descriptors/DescriptorAllocator.h"
#include "VulkanWrapper/Descriptors/DescriptorSet.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

namespace Internal {
class DescriptorPoolImpl
    : public ObjectWithUniqueHandle<vk::UniqueDescriptorPool> {
  public:
    DescriptorPoolImpl(
        vk::UniqueDescriptorPool pool, std::shared_ptr<const Device> device,
        const std::shared_ptr<const DescriptorSetLayout> &layout);

    std::optional<vk::DescriptorSet> allocate_set();

  private:
    uint32_t m_number_allocation = 0;
    std::vector<vk::DescriptorSet> m_sets;
};
} // namespace Internal

class DescriptorPool {
  public:
    DescriptorPool(std::shared_ptr<const Device> device,
                   std::shared_ptr<const DescriptorSetLayout> layout);

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const noexcept;

    [[nodiscard]] DescriptorSet
    allocate_set(const DescriptorAllocator &descriptorAllocator) noexcept;

  private:
    vk::DescriptorSet allocate_descriptor_set_from_last_pool();

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
    std::vector<Internal::DescriptorPoolImpl> m_descriptor_pools;
    std::unordered_map<DescriptorAllocator, DescriptorSet> m_sets;
};

class DescriptorPoolBuilder {
  public:
    DescriptorPoolBuilder(
        std::shared_ptr<const Device> device,
        const std::shared_ptr<const DescriptorSetLayout> &layout);

    DescriptorPool build();

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
};
} // namespace vw
