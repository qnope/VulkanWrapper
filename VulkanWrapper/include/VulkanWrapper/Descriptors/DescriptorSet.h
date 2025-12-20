#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Synchronization/ResourceTracker.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <vector>

namespace vw {

class DescriptorSet : public ObjectWithHandle<vk::DescriptorSet> {
  public:
    DescriptorSet(vk::DescriptorSet handle,
                  std::vector<Barrier::ResourceState> resources) noexcept;

    const std::vector<Barrier::ResourceState> &resources() const noexcept;

  private:
    std::vector<Barrier::ResourceState> m_resources;
};

} // namespace vw
