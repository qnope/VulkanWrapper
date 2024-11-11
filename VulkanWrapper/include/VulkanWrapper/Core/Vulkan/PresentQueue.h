#pragma once

namespace vw {
class PresentQueue {
    friend class DeviceFinder;

  private:
    PresentQueue(vk::Queue queue) noexcept;

  private:
    vk::Queue m_queue;
};
} // namespace vw
