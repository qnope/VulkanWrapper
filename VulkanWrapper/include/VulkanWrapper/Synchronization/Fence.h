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
    Fence(const Device &device, vk::UniqueFence fence);

  private:
    const Device &m_device;
};

class FenceBuilder {
  public:
    FenceBuilder(const Device &device);
    Fence build() &&;

  private:
    const Device &m_device;
};
} // namespace vw
