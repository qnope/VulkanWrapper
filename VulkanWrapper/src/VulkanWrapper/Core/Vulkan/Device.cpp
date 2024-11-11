#include "VulkanWrapper/Core/Vulkan/Device.h"

#include "VulkanWrapper/Core/Vulkan/Queue.h"

namespace vw {

Device::Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
               std::vector<Queue> queues,
               std::optional<PresentQueue> presentQueue) noexcept
    : ObjectWithUniqueHandle<vk::UniqueDevice>{std::move(device)}
    , m_physicalDevice{physicalDevice}
    , m_queues{std::move(queues)}
    , m_presentQueue{std::move(presentQueue)} {}

} // namespace vw
