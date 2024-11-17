#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Utils/exceptions.h"
#include "VulkanWrapper/Core/Vulkan/ObjectWithHandle.h"
#include "VulkanWrapper/Core/Vulkan/PresentQueue.h"

namespace vw {
using DeviceCreationException = TaggedException<struct DeviceCreationTag>;

class Device : public ObjectWithUniqueHandle<vk::UniqueDevice> {
    friend class DeviceFinder;

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
