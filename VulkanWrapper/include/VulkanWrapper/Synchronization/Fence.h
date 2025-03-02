#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;

  public:
    Fence(const Fence &) = delete;
    Fence(Fence &&) = default;
    void wait() const;
    void reset() const;

    ~Fence();

  private:
    Fence(const Device &device, vk::UniqueFence fence);

  private:
    const Device *m_device;
};

class FenceBuilder {
  public:
    FenceBuilder(const Device &device);
    Fence build() &&;

  private:
    const Device &m_device;
};
} // namespace vw
