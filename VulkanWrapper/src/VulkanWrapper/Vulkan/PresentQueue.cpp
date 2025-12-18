#include "VulkanWrapper/Vulkan/PresentQueue.h"

#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"

namespace vw {
PresentQueue::PresentQueue(vk::Queue queue) noexcept
    : m_queue{queue} {}

PresentResult PresentQueue::present(const Swapchain &swapchain, uint32_t index,
                                    const Semaphore &semaphore) const {
    const auto swapchainHandle = swapchain.handle();
    const auto semaphoreHandle = semaphore.handle();
    const auto presentInfo = vk::PresentInfoKHR()
                                 .setSwapchains(swapchainHandle)
                                 .setImageIndices(index)
                                 .setWaitSemaphores(semaphoreHandle);

    auto result = m_queue.presentKHR(presentInfo);

    switch (result) {
    case vk::Result::eSuccess:
        return PresentResult::Success;
    case vk::Result::eSuboptimalKHR:
        return PresentResult::Suboptimal;
    case vk::Result::eErrorOutOfDateKHR:
        return PresentResult::OutOfDate;
    default:
        return PresentResult::OutOfDate;
    }
}

} // namespace vw
