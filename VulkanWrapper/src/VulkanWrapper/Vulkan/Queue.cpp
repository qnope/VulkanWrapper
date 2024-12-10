#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {
Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}

void Queue::submit(const std::vector<vk::SubmitInfo> &infos,
                   const Fence *fence) const {
    vk::Fence fenceHandle;
    if (fence)
        fenceHandle = fence->handle();
    m_queue.submit(infos, fenceHandle);
}
} // namespace vw
