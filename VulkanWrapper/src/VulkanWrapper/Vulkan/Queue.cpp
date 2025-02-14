#include "VulkanWrapper/Vulkan/Queue.h"

#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Utils/Algos.h"

namespace vw {

Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}

void Queue::submit(std::span<const vk::CommandBuffer> commandBuffers,
                   std::span<const vk::PipelineStageFlags> waitStages,
                   std::span<const vk::Semaphore> waitSemaphores,
                   std::span<const vk::Semaphore> signalSemaphores,
                   const Fence *fence) const {
    const auto infos = vk::SubmitInfo()
                           .setCommandBuffers(commandBuffers)
                           .setWaitDstStageMask(waitStages)
                           .setWaitSemaphores(waitSemaphores)
                           .setSignalSemaphores(signalSemaphores);

    vk::Fence fenceHandle;
    if (fence)
        fenceHandle = fence->handle();
    m_queue.submit(infos, fenceHandle);
}

} // namespace vw
