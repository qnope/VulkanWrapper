#include "VulkanWrapper/Vulkan/Device.h"

#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

Device::Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
               std::vector<Queue> queues,
               std::optional<PresentQueue> presentQueue) noexcept
    : ObjectWithUniqueHandle<vk::UniqueDevice>{std::move(device)}
    , m_physicalDevice{physicalDevice}
    , m_queues{std::move(queues)}
    , m_presentQueue{presentQueue} {
    for (auto &queue : m_queues) {
        queue.m_device = this;
    }
}

Queue &Device::graphicsQueue() { return m_queues[0]; }

const PresentQueue &Device::presentQueue() const {
    assert(m_presentQueue);
    return m_presentQueue.value();
}

void Device::wait_idle() const { std::ignore = handle().waitIdle(); }

vk::PhysicalDevice Device::physical_device() const { return m_physicalDevice; }

} // namespace vw
