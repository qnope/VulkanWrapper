#include "VulkanWrapper/Vulkan/DeviceFinder.h"

#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

DeviceFinder::DeviceFinder(std::span<PhysicalDevice> physicalDevices) noexcept {
    for (auto &device : physicalDevices) {
        PhysicalDeviceInformation information{.device = device};
        auto properties = device.queueFamilyProperties();
        for (const auto &property : properties) {
            information.queuesInformation.emplace_back(0, property.queueCount,
                                                       property.queueFlags);
        }

        information.availableExtensions = device.extensions();

        m_physicalDevicesInformation.push_back(information);
    }
}

DeviceFinder &DeviceFinder::with_queue(vk::QueueFlags queueFlags) {
    auto queueFamilyHandled = [queueFlags](const QueueFamilyInformation &info) {
        return info.numberAsked < info.numberAvailable &&
               (queueFlags & info.flags) == queueFlags;
    };

    auto doesNotHandleQueue = [&](const PhysicalDeviceInformation &device) {
        return std::ranges::none_of(device.queuesInformation,
                                    queueFamilyHandled);
    };

    std::erase_if(m_physicalDevicesInformation, doesNotHandleQueue);

    for (auto &information : m_physicalDevicesInformation) {
        auto index =
            index_if(information.queuesInformation, queueFamilyHandled);
        if (!index) {
            throw LogicException::invalid_state(
                "Internal error: queue family should exist after filtering");
        }
        ++information.queuesInformation[*index].numberAsked;
        ++information.numberOfQueuesToCreate[*index];
    }

    return *this;
}

DeviceFinder &DeviceFinder::with_presentation(vk::SurfaceKHR surface) noexcept {
    using namespace std::ranges;

    constexpr auto swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    auto whichFamilyHandlePresentation =
        [&](const PhysicalDeviceInformation &information)
        -> std::optional<int> {
        const auto &queues = information.queuesInformation;
        for (int i = 0; i < queues.size(); ++i) {
            auto [result, supportPresentation] =
                information.device.device().getSurfaceSupportKHR(i, surface);
            if (result == vk::Result::eSuccess &&
                static_cast<bool>(supportPresentation)) {
                return i;
            }
        }
        return std::nullopt;
    };

    auto doesNotHandlePresentation =
        [&](const PhysicalDeviceInformation &information) {
            return !information.availableExtensions.contains(
                       swapchainExtension) ||
                   !whichFamilyHandlePresentation(information).has_value();
        };

    std::erase_if(m_physicalDevicesInformation, doesNotHandlePresentation);

    for (auto &information : m_physicalDevicesInformation) {
        information.presentationFamilyIndex =
            whichFamilyHandlePresentation(information);
        information.extensions.push_back(swapchainExtension);
    }

    return *this;
}

DeviceFinder &DeviceFinder::with_synchronization_2() noexcept {
    remove_device_not_supporting_extension(
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    for (auto &information : m_physicalDevicesInformation) {
        information.extensions.push_back(
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
    }

    m_features.get<vk::PhysicalDeviceSynchronization2Features>()
        .setSynchronization2(1U);
    return *this;
}

DeviceFinder &DeviceFinder::with_ray_tracing() noexcept {
    constexpr std::array extensions = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME};

    for (const auto &extension : extensions)
        remove_device_not_supporting_extension(extension);

    for (auto &information : m_physicalDevicesInformation)
        information.extensions.append_range(extensions);

    m_features.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>()
        .setAccelerationStructure(1U);
    m_features.get<vk::PhysicalDeviceRayQueryFeaturesKHR>().setRayQuery(1u);
    m_features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>()
        .setRayTracingPipeline(1U);

    return *this;
}

DeviceFinder &DeviceFinder::with_dynamic_rendering() noexcept {
    remove_device_not_supporting_extension(
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    for (auto &information : m_physicalDevicesInformation) {
        information.extensions.push_back(
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    }

    m_features.get<vk::PhysicalDeviceDynamicRenderingFeatures>()
        .setDynamicRendering(1U);
    return *this;
}

std::optional<PhysicalDevice> DeviceFinder::get() noexcept {
    if (m_physicalDevicesInformation.empty()) {
        return {};
    }
    return std::ranges::max_element(m_physicalDevicesInformation, std::less<>{},
                                    &PhysicalDeviceInformation::device)
        ->device;
}

void DeviceFinder::remove_device_not_supporting_extension(
    const char *extension) {
    auto need_to_remove =
        [extension](const PhysicalDeviceInformation &information) {
            return !information.availableExtensions.contains(extension);
        };
    std::erase_if(m_physicalDevicesInformation, need_to_remove);
}

std::shared_ptr<Device> DeviceFinder::build() {
    if (m_physicalDevicesInformation.empty()) {
        throw LogicException::invalid_state(
            "No suitable GPU found matching requested features");
    }

    auto information =
        *std::ranges::max_element(m_physicalDevicesInformation, std::less<>{},
                                  &PhysicalDeviceInformation::device);

    std::cout << "Take "
              << information.device.device().getProperties().deviceName
              << std::endl;

    if (std::ranges::find(information.availableExtensions,
                          "VK_KHR_portability_subset") !=
        information.availableExtensions.end()) {
        information.extensions.push_back("VK_KHR_portability_subset");
    }

    vk::DeviceCreateInfo info;
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;

    constexpr float priority = {1.0F};

    for (auto [familyIndex, queueCount] : information.numberOfQueuesToCreate) {
        auto queueInfo = vk::DeviceQueueCreateInfo()
                             .setQueuePriorities(priority)
                             .setQueueFamilyIndex(familyIndex)
                             .setQueueCount(queueCount);
        queueInfos.push_back(queueInfo);
    }

    if (information.presentationFamilyIndex) {
        if (information
                .numberOfQueuesToCreate[*information.presentationFamilyIndex] ==
            0) {
            auto queueInfo =
                vk::DeviceQueueCreateInfo()
                    .setQueuePriorities(priority)
                    .setQueueFamilyIndex(*information.presentationFamilyIndex)
                    .setQueueCount(1);
            queueInfos.push_back(queueInfo);
        }
    }

    info.setQueueCreateInfos(queueInfos);
    info.setPEnabledExtensionNames(information.extensions);

    m_features.get<vk::PhysicalDeviceVulkan12Features>().setBufferDeviceAddress(
        1U);

    // Unlink ray tracing feature structures if ray tracing wasn't requested
    // This prevents validation errors about features in pNext without
    // extensions
    auto &accelFeatures =
        m_features.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
    auto &rayQueryFeatures =
        m_features.get<vk::PhysicalDeviceRayQueryFeaturesKHR>();
    auto &rtPipelineFeatures =
        m_features.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();

    if (!accelFeatures.accelerationStructure) {
        m_features.unlink<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
    }
    if (!rayQueryFeatures.rayQuery) {
        m_features.unlink<vk::PhysicalDeviceRayQueryFeaturesKHR>();
    }
    if (!rtPipelineFeatures.rayTracingPipeline) {
        m_features.unlink<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();
    }

    info.setPNext(&m_features.get<vk::PhysicalDeviceFeatures2>());

    auto device = check_vk(information.device.device().createDeviceUnique(info),
                           "Failed to create logical device");

    std::vector<Queue> queues;

    for (auto [familyIndex, queueCount] : information.numberOfQueuesToCreate) {
        for (int i = 0; i < queueCount; ++i) {
            Queue queue(device->getQueue(familyIndex, i),
                        information.queuesInformation[familyIndex].flags);
            queues.push_back(std::move(queue));
        }
    }

    std::optional<PresentQueue> presentQueue;

    if (information.presentationFamilyIndex) {
        presentQueue = PresentQueue(
            device->getQueue(*information.presentationFamilyIndex, 0));
    }

    return std::shared_ptr<Device>(new Device(std::move(device),
                                              information.device.device(),
                                              std::move(queues), presentQueue));
}

} // namespace vw
