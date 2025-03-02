#include "VulkanWrapper/Vulkan/Queue.h"

#include "VulkanWrapper/Utils/Algos.h"

namespace vw {

Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}

void Queue::enqueue_command_buffer(vk::CommandBuffer command_buffer) {
    const int index = command_buffer_index();

    m_command_buffers.emplace_back(index, command_buffer);
}

void Queue::enqueue_command_buffers(
    std::span<const vk::CommandBuffer> command_buffers) {
    for (auto buffer : command_buffers)
        enqueue_command_buffer(buffer);
}

int Queue::command_buffer_index() const {
    int index = m_command_buffers.size();
    for (auto &[idx, pool, cmd_buffers] : m_command_buffers_and_living_pools) {
        index += cmd_buffers.size();
    }
    return index;
}

FenceAndLivingPools
Queue::submit(std::span<const vk::PipelineStageFlags> waitStages,
              std::span<const vk::Semaphore> waitSemaphores,
              std::span<const vk::Semaphore> signalSemaphores) {
    std::map<int, vk::CommandBuffer> ordered_cmd_buffers;

    for (auto &[idx, buffer] : m_command_buffers)
        ordered_cmd_buffers.emplace(idx, buffer);

    for (const auto &[idx, pool, buffers] :
         m_command_buffers_and_living_pools) {
        auto index = idx;
        for (auto cmd_buffer : buffers)
            ordered_cmd_buffers.emplace(index++, cmd_buffer);
    }

    auto fence = FenceBuilder(*m_device).build();

    std::vector<vk::CommandBuffer> cmd_buffers =
        ordered_cmd_buffers | std::views::values | to<std::vector>;

    const auto infos = vk::SubmitInfo()
                           .setCommandBuffers(cmd_buffers)
                           .setWaitDstStageMask(waitStages)
                           .setWaitSemaphores(waitSemaphores)
                           .setSignalSemaphores(signalSemaphores);

    m_queue.submit(infos, fence.handle());

    std::vector<CommandPool> living_pools =
        m_command_buffers_and_living_pools | std::views::values |
        std::views::as_rvalue | to<std::vector>;

    return {std::move(fence), std::move(living_pools)};
}

} // namespace vw
