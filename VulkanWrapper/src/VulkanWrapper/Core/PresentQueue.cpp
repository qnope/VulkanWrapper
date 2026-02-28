module vw.vulkan;
import vulkan3rd;

namespace vw {
PresentQueue::PresentQueue(vk::Queue queue) noexcept
    : m_queue{queue} {}

void PresentQueue::present(vk::SwapchainKHR swapchain, uint32_t index,
                           vk::Semaphore semaphore) const {
    const auto presentInfo = vk::PresentInfoKHR()
                                 .setSwapchains(swapchain)
                                 .setImageIndices(index)
                                 .setWaitSemaphores(semaphore);

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
