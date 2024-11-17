#include "VulkanWrapper/Vulkan/PresentQueue.h"

namespace vw {
PresentQueue::PresentQueue(vk::Queue queue) noexcept
    : m_queue{queue} {}
} // namespace vw
