#include "VulkanWrapper/Vulkan/Queue.h"

#include "VulkanWrapper/Synchronization/Fence.h"

namespace vw {

Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}

void Queue::enqueue_command_buffer(vk::CommandBuffer command_buffer) {
    m_command_buffers.push_back(command_buffer);
}

void Queue::enqueue_command_buffers(
    std::span<const vk::CommandBuffer> command_buffers) {
    for (auto buffer : command_buffers) {
        enqueue_command_buffer(buffer);
    }
}

Fence Queue::submit(std::span<const vk::PipelineStageFlags> waitStages,
                    std::span<const vk::Semaphore> waitSemaphores,
                    std::span<const vk::Semaphore> signalSemaphores) {
    auto fence = FenceBuilder(m_device).build();

    std::vector<vk::CommandBuffer> cmd_buffers =
        std::exchange(m_command_buffers, {});

    const auto infos = vk::SubmitInfo()
                           .setCommandBuffers(cmd_buffers)
                           .setWaitDstStageMask(waitStages)
                           .setWaitSemaphores(waitSemaphores)
                           .setSignalSemaphores(signalSemaphores);

    std::ignore = m_queue.submit(infos, fence.handle());

    return fence;
}

} // namespace vw
