#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"
#include <memory>

namespace vw {
using DeviceCreationException = TaggedException<struct DeviceCreationTag>;

// Private implementation - definition in Device.cpp
struct DeviceImpl;

class Device {
    friend class DeviceFinder;

  public:
    Queue &graphicsQueue();
    [[nodiscard]] const PresentQueue &presentQueue() const;
    void wait_idle() const;
    [[nodiscard]] vk::PhysicalDevice physical_device() const;
    [[nodiscard]] vk::Device handle() const;

    // Default constructor creates an empty device (for Queue initialization)
    Device(const Device &) = default;
    Device &operator=(const Device &) = default;
    Device(Device &&) noexcept = default;
    Device &operator=(Device &&) noexcept = default;

  private:
    Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
           std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

    std::shared_ptr<DeviceImpl> m_impl;
};

} // namespace vw
