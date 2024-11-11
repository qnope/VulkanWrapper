#pragma once

namespace r3d {

class Queue {
    friend class DeviceFinder;

  public:
  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

  private:
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

} // namespace r3d
