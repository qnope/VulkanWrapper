#pragma once

#include "VulkanWrapper/Descriptors/DescriptorPool.h"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Memory/StagingBufferManager.h"

namespace vw {
class MeshManager {
  public:
    MeshManager(const Device &device, const Allocator &allocator);

  private:
    StagingBufferManager m_staging_buffer_manager;
    std::shared_ptr<DescriptorSetLayout> m_descriptor_set_layout;
    DescriptorPool m_descriptor_pool;
};
} // namespace vw
