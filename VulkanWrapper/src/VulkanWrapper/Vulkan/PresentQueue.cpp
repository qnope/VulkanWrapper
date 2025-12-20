#include "VulkanWrapper/Vulkan/PresentQueue.h"

#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Swapchain.h"

namespace vw {
PresentQueue::PresentQueue(vk::Queue queue) noexcept
    : m_queue{queue} {}

void PresentQueue::present(const Swapchain &swapchain, uint32_t index,
                           const Semaphore &semaphore) const {
    const auto swapchainHandle = swapchain.handle();
    const auto semaphoreHandle = semaphore.handle();
    const auto presentInfo = vk::PresentInfoKHR()
                                 .setSwapchains(swapchainHandle)
                                 .setImageIndices(index)
                                 .setWaitSemaphores(semaphoreHandle);

    auto result = m_queue.presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        throw SwapchainException(result, "Swapchain needs recreation");
    }

    if (result != vk::Result::eSuccess) {
        throw VulkanException(result, "Failed to present swapchain image");
    }
}

} // namespace vw
