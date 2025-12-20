#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"

namespace vw {
class PresentQueue {
    friend class DeviceFinder;

  public:
    void present(const Swapchain &swapchain, uint32_t imageIndex,
                 const Semaphore &waitSemaphore) const;

  private:
    PresentQueue(vk::Queue queue) noexcept;

    vk::Queue m_queue;
};
} // namespace vw
