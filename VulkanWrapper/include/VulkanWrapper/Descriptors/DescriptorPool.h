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

class DescriptorPool : public ObjectWithUniqueHandle<vk::UniqueDescriptorPool> {
  public:
    DescriptorPool(const Device &device,
                   std::shared_ptr<const DescriptorSetLayout> layout,
                   vk::UniqueDescriptorPool pool);

    vk::DescriptorSet
    allocate_set(const DescriptorAllocator &descriptorAllocator);

  private:
    const Device *m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
    std::unordered_map<DescriptorAllocator, vk::DescriptorSet> m_sets;
};

class DescriptorPoolBuilder {
  public:
    DescriptorPoolBuilder(const Device &device,
                          const std::shared_ptr<DescriptorSetLayout> &layout,
                          int number_of_set);

    DescriptorPool build() &&;

  private:
    const Device *m_device;
    int m_number_of_set;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
};
} // namespace vw
