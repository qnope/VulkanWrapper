#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;
    friend class Queue;

  public:
    void wait() const;
    void reset() const;

    ~Fence();

  private:
    // The fence is bound to the Queue
    Fence(const Device &device, vk::UniqueFence fence);
    Fence(const Fence &) = delete;
    Fence(Fence &&) = default;

  private:
    const Device *m_device;
};

class FenceBuilder {
    friend class Queue;
    FenceBuilder(const Device &device);
    Fence build() &&;

  private:
    const Device &m_device;
};
} // namespace vw
