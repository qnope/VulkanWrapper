#pragma once

#include "3DRenderer/3rd_party.h"

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
