#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"

namespace vw {
using DeviceCreationException = TaggedException<struct DeviceCreationTag>;

class Device : public ObjectWithUniqueHandle<vk::UniqueDevice> {
    friend class DeviceFinder;

  public:
    const Queue &graphicsQueue() const;
    const PresentQueue &presentQueue() const;

  private:
    Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
           std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

  private:
    vk::PhysicalDevice m_physicalDevice;
    std::vector<Queue> m_queues;
    std::optional<PresentQueue> m_presentQueue;
};

} // namespace vw
