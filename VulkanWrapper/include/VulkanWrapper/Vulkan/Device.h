#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"
#include <memory>

namespace vw {

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

    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    Device(Device &&) = delete;
    Device &operator=(Device &&) = delete;

  private:
    Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
           std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

    std::shared_ptr<DeviceImpl> m_impl;
};

} // namespace vw
