#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

using CommandPoolCreationException =
    vw::TaggedException<struct CommandPoolCreationTag>;

using CommandBufferAllocationException =
    vw::TaggedException<struct CommandBufferAllocationTag>;

class CommandPool : public ObjectWithUniqueHandle<vk::UniqueCommandPool> {
    friend class CommandPoolBuilder;

  public:
    std::vector<vk::CommandBuffer> allocate(std::size_t number);

  private:
    CommandPool(Device &device, vk::UniqueCommandPool commandPool);

  private:
    Device &m_device;
};

class CommandPoolBuilder {
  public:
    CommandPool build(Device &device) &&;

  private:
};

} // namespace vw
