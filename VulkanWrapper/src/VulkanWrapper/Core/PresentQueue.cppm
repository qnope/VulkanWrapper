export module vw.vulkan:present_queue;
import vulkan3rd;

export namespace vw {

class PresentQueue {
    friend class DeviceFinder;

  public:
    void present(vk::SwapchainKHR swapchain, uint32_t imageIndex,
                 vk::Semaphore waitSemaphore) const;

  private:
    PresentQueue(vk::Queue queue) noexcept;

    vk::Queue m_queue;
};
} // namespace vw
