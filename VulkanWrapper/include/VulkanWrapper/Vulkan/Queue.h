#pragma once

#include "VulkanWrapper/fwd.h"

namespace vw {

class Queue {
    friend class DeviceFinder;

  public:
      void submit(std::span<const vk::CommandBuffer> commandBuffers,
                  std::span<const vk::PipelineStageFlags> waitStage,
                  std::span<const vk::Semaphore> waitSemaphores,
                  std::span<const vk::Semaphore> signalSemaphores,
                  const Fence *fence) const;

  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

  private:
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

} // namespace vw
