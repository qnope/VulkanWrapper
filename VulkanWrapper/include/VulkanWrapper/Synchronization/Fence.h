#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;

  public:
    void wait() const;
    void reset() const;

  private:
    Fence(Device &device, vk::UniqueFence fence);

  private:
    Device &m_device;
};

class FenceBuilder {
  public:
    Fence build(Device &device) &&;
};
} // namespace vw
