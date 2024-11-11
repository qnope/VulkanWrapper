#include "VulkanWrapper/Core/Vulkan/Queue.h"

namespace vw {
Queue::Queue(vk::Queue queue, vk::QueueFlags type) noexcept
    : m_queue{queue}
    , m_queueFlags{type} {}
} // namespace vw
