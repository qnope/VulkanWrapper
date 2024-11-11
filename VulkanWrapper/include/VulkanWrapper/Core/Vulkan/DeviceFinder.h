#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Vulkan/PhysicalDevice.h"

namespace r3d {
class DeviceFinder {
  public:
    DeviceFinder(std::vector<PhysicalDevice> physicalDevices) noexcept;

    DeviceFinder &&withQueue(vk::QueueFlags queueFlags) && noexcept;
    DeviceFinder &&withPresentQueue(vk::SurfaceKHR surface) && noexcept;

    Device build() &&;
    std::optional<PhysicalDevice> get() && noexcept;

  private:
    struct QueueFamilyInformation {
        int numberAsked = 0;
        uint32_t numberAvailable;
        vk::QueueFlags flags;
    };

    struct PhysicalDeviceInformation {
        PhysicalDevice device;
        std::set<std::string> availableExtensions;
        std::vector<QueueFamilyInformation> queuesInformation;
        std::map<int, int> numberOfQueuesToCreate;
        std::optional<int> presentationFamilyIndex;
        std::vector<const char *> extensions;
    };
    std::vector<PhysicalDeviceInformation> m_physicalDevicesInformation;
};
} // namespace r3d
