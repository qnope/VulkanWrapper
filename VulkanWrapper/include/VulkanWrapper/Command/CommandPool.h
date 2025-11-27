#pragma once
#include "VulkanWrapper/3rd_party.h"

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
    CommandPool(const Device &device, vk::UniqueCommandPool commandPool);

    const Device *m_device;
};

class CommandPoolBuilder {
  public:
    CommandPoolBuilder(const Device &device);
    CommandPool build() &&;

  private:
    const Device *m_device;
};

} // namespace vw
