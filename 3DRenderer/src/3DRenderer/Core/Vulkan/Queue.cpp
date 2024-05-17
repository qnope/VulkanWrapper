#include "3DRenderer/Core/Vulkan/Queue.h"

namespace r3d {
Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}
} // namespace r3d
