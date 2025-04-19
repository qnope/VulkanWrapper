#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

namespace vw {
using DeviceNotFoundException = TaggedException<struct DeviceNotFoundTag>;

class DeviceFinder {
  public:
    DeviceFinder(std::span<PhysicalDevice> physicalDevices) noexcept;

    DeviceFinder &&with_queue(vk::QueueFlags queueFlags) && noexcept;
    DeviceFinder &&with_presentation(vk::SurfaceKHR surface) && noexcept;
    DeviceFinder &&with_synchronization_2() && noexcept;
    DeviceFinder &&with_ray_tracing() && noexcept;

    Device build() &&;
    std::optional<PhysicalDevice> get() && noexcept;

  private:
    void remove_device_not_supporting_extension(const char *extension);

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

    vk::PhysicalDeviceFeatures2 m_features;
    vk::PhysicalDeviceSynchronization2Features m_synchronisation_2_enabled;
};
} // namespace vw
