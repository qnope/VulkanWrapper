#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

namespace vw {

class DeviceFinder {
  public:
    DeviceFinder(std::span<PhysicalDevice> physicalDevices) noexcept;

    DeviceFinder &with_queue(vk::QueueFlags queueFlags);
    DeviceFinder &with_presentation(vk::SurfaceKHR surface) noexcept;
    DeviceFinder &with_synchronization_2() noexcept;
    DeviceFinder &with_ray_tracing() noexcept;
    DeviceFinder &with_dynamic_rendering() noexcept;
    DeviceFinder &with_descriptor_indexing() noexcept;
    DeviceFinder &with_scalar_block_layout() noexcept;

    std::shared_ptr<Device> build();
    std::optional<PhysicalDevice> get() noexcept;

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

    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceSynchronization2Features,
                       vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
                       vk::PhysicalDeviceRayQueryFeaturesKHR,
                       vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                       vk::PhysicalDeviceDynamicRenderingFeatures>
        m_features;
};
} // namespace vw
