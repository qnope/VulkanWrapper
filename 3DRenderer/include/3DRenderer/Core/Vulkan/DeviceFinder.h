#pragma once

#include "3DRenderer/3rd_party.h"
#include "3DRenderer/Core/Vulkan/PhysicalDevice.h"
#include "3DRenderer/Core/fwd.h"

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
        std::vector<QueueFamilyInformation> queuesInformation;
        std::map<int, int> numberOfQueuesToCreate;
        std::optional<int> presentationFamilyIndex;
    };
    std::vector<PhysicalDeviceInformation> m_physicalDevicesInformation;
};
} // namespace r3d
