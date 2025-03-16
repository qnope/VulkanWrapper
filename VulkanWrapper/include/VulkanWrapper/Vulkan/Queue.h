#pragma once

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Synchronization/Fence.h"

namespace vw {

class Queue {
    friend class DeviceFinder;
    friend class Device;

  public:
    void enqueue_command_buffer(vk::CommandBuffer command_buffer);
    void
    enqueue_command_buffers(std::span<const vk::CommandBuffer> command_buffers);

    Fence submit(std::span<const vk::PipelineStageFlags> waitStage,
                 std::span<const vk::Semaphore> waitSemaphores,
                 std::span<const vk::Semaphore> signalSemaphores);

  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

    std::vector<vk::CommandBuffer> m_command_buffers;

    Device *m_device = nullptr;
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

} // namespace vw
