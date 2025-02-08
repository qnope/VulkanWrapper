#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

namespace vw {
using DeviceNotFoundException = TaggedException<struct DeviceNotFoundTag>;

class DeviceFinder {
  public:
    DeviceFinder(std::vector<PhysicalDevice> physicalDevices) noexcept;

    DeviceFinder &&with_queue(vk::QueueFlags queueFlags) && noexcept;
    DeviceFinder &&with_presentation(vk::SurfaceKHR surface) && noexcept;

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
} // namespace vw
