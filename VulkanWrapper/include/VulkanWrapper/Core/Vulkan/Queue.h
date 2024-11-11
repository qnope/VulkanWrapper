#pragma once

namespace vw {

class Queue {
    friend class DeviceFinder;

  public:
  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

  private:
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

} // namespace vw
