#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;
    friend class Queue;

  public:
    void wait() const;
    void reset() const;

    Fence(const Fence &) = delete;
    Fence(Fence &&) noexcept = default;
    Fence &operator=(const Fence &) = delete;
    Fence &operator=(Fence &&) noexcept = default;
    ~Fence();

  private:
    // The fence is bound to the Queue
    Fence(const Device &device, vk::UniqueFence fence);

    const Device *m_device;
};

class FenceBuilder {
    friend class Queue;
    FenceBuilder(const Device &device);
    Fence build() &&;

  private:
    const Device *m_device;
};
} // namespace vw
