#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {
using DeviceCreationException = TaggedException<struct DeviceCreationTag>;

class Device : public ObjectWithUniqueHandle<vk::UniqueDevice> {
    friend class DeviceFinder;

  public:
    Queue &graphicsQueue();
    [[nodiscard]] const PresentQueue &presentQueue() const;
    void wait_idle() const;
    [[nodiscard]] vk::PhysicalDevice physical_device() const;

  private:
    Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
           std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

    vk::PhysicalDevice m_physicalDevice;
    std::vector<Queue> m_queues;
    std::optional<PresentQueue> m_presentQueue;
};

} // namespace vw
