#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class CommandPool : public ObjectWithUniqueHandle<vk::UniqueCommandPool> {
    friend class CommandPoolBuilder;

  public:
    std::vector<vk::CommandBuffer> allocate(std::size_t number);

  private:
    CommandPool(std::shared_ptr<const Device> device,
                vk::UniqueCommandPool commandPool);

    std::shared_ptr<const Device> m_device;
};

class CommandPoolBuilder {
  public:
    CommandPoolBuilder(std::shared_ptr<const Device> device);

    /** @brief Allow individual command buffers to be reset */
    CommandPoolBuilder& with_reset_command_buffer();

    CommandPool build();

  private:
    std::shared_ptr<const Device> m_device;
    vk::CommandPoolCreateFlags m_flags{};
};

} // namespace vw
