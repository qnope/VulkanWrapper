#include "VulkanWrapper/Vulkan/Queue.h"

#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Synchronization/Semaphore.h"
#include "VulkanWrapper/Utils/Algos.h"

namespace vw {

Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}

void Queue::enqueue_command_buffer(vk::CommandBuffer command_buffer) {
    m_command_buffers.push_back(command_buffer);
}

void Queue::enqueue_command_buffers(
    std::span<const vk::CommandBuffer> command_buffers) {
    m_command_buffers.insert(m_command_buffers.end(), command_buffers.begin(),
                             command_buffers.end());
}

FenceAndLivingPools
Queue::submit(std::span<const vk::PipelineStageFlags> waitStages,
              std::span<const vk::Semaphore> waitSemaphores,
              std::span<const vk::Semaphore> signalSemaphores) {
    std::vector<vk::CommandBuffer> cmd_buffers = std::move(m_command_buffers);

    for (const auto &buffers :
         m_command_buffers_and_living_pools | std::views::values)
        cmd_buffers.insert(cmd_buffers.end(), buffers.begin(), buffers.end());

    auto fence = FenceBuilder(*m_device).build();

    const auto infos = vk::SubmitInfo()
                           .setCommandBuffers(cmd_buffers)
                           .setWaitDstStageMask(waitStages)
                           .setWaitSemaphores(waitSemaphores)
                           .setSignalSemaphores(signalSemaphores);

    m_queue.submit(infos, fence.handle());

    std::vector<CommandPool> living_pools =
        m_command_buffers_and_living_pools | std::views::keys |
        std::views::as_rvalue | to<std::vector>;

    return {std::move(fence), std::move(living_pools)};
}

} // namespace vw
