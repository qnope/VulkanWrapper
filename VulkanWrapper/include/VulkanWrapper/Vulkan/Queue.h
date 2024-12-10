#pragma once

#include "VulkanWrapper/Synchronization/Fence.h"

namespace vw {

class Queue {
    friend class DeviceFinder;

  public:
    void submit(const std::vector<vk::SubmitInfo> &infos,
                const Fence *fence) const;

  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

  private:
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

} // namespace vw
