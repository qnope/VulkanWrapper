#include "VulkanWrapper/Core/Vulkan/PresentQueue.h"

namespace r3d {
PresentQueue::PresentQueue(vk::Queue queue) noexcept
    : m_queue{queue} {}
} // namespace r3d
