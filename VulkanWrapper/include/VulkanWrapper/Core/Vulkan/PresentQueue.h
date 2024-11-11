#pragma once

namespace r3d {
class PresentQueue {
    friend class DeviceFinder;

  private:
    PresentQueue(vk::Queue queue) noexcept;

  private:
    vk::Queue m_queue;
};
} // namespace r3d
