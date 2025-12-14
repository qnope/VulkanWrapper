#include "VulkanWrapper/Vulkan/Device.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

struct DeviceImpl {
    vk::UniqueDevice device;
    vk::PhysicalDevice physicalDevice;
    std::vector<Queue> queues;
    std::optional<PresentQueue> presentQueue;
};

Device::Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
               std::vector<Queue> queues,
               std::optional<PresentQueue> presentQueue) noexcept
    : m_impl{std::make_shared<DeviceImpl>(
          DeviceImpl{.device = std::move(device),
                     .physicalDevice = physicalDevice,
                     .queues = std::move(queues),
                     .presentQueue = std::move(presentQueue)})} {
    // Set the device for each queue
    for (auto &queue : m_impl->queues) {
        queue.m_device = handle();
    }
}

Queue &Device::graphicsQueue() { return m_impl->queues[0]; }

const PresentQueue &Device::presentQueue() const {
    if (!m_impl->presentQueue) {
        throw LogicException::invalid_state(
            "Device was not created with presentation support");
    }
    return m_impl->presentQueue.value();
}

void Device::wait_idle() const { std::ignore = m_impl->device->waitIdle(); }

vk::PhysicalDevice Device::physical_device() const {
    return m_impl->physicalDevice;
}

vk::Device Device::handle() const { return m_impl->device.get(); }

} // namespace vw
