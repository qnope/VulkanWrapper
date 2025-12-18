#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"

namespace vw {

enum class PresentResult {
    Success,
    OutOfDate,
    Suboptimal
};

class PresentQueue {
    friend class DeviceFinder;

  public:
    [[nodiscard]] PresentResult present(const Swapchain &swapchain, uint32_t imageIndex,
                                        const Semaphore &waitSemaphore) const;

  private:
    PresentQueue(vk::Queue queue) noexcept;

    vk::Queue m_queue;
};
} // namespace vw
