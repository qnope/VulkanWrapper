#include "VulkanWrapper/Vulkan/DeviceFinder.h"

#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/PhysicalDevice.h"
#include "VulkanWrapper/Vulkan/PresentQueue.h"
#include "VulkanWrapper/Vulkan/Queue.h"

namespace vw {

DeviceFinder::DeviceFinder(std::span<PhysicalDevice> physicalDevices) noexcept {
    std::cout << physicalDevices.size() << std::endl;
    for (auto &device : physicalDevices) {
        std::cout << "Found: " << device.device().getProperties().deviceName
                  << std::endl;
        PhysicalDeviceInformation information{device};
        auto properties = device.queueFamilyProperties();
        for (const auto &property : properties) {
            information.queuesInformation.emplace_back(0, property.queueCount,
                                                       property.queueFlags);
        }

        information.availableExtensions = device.extensions();

        m_physicalDevicesInformation.push_back(information);
    }
}

DeviceFinder &&DeviceFinder::with_queue(vk::QueueFlags queueFlags) && noexcept {
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
            *index_if(information.queuesInformation, queueFamilyHandled);
        ++information.queuesInformation[index].numberAsked;
        ++information.numberOfQueuesToCreate[index];
    }

    return std::move(*this);
}

DeviceFinder &&
DeviceFinder::with_presentation(vk::SurfaceKHR surface) && noexcept {
    using namespace std::ranges;

    constexpr auto swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    auto whichFamilyHandlePresentation =
        [&](const PhysicalDeviceInformation &information)
        -> std::optional<int> {
        auto &queues = information.queuesInformation;
        for (int i = 0; i < queues.size(); ++i) {
            auto [result, supportPresentation] =
                information.device.device().getSurfaceSupportKHR(i, surface);
            if (result == vk::Result::eSuccess && supportPresentation == true)
                return i;
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
            *whichFamilyHandlePresentation(information);
        information.extensions.push_back(swapchainExtension);
    }

    return std::move(*this);
}

DeviceFinder &&DeviceFinder::with_synchronization_2() && noexcept {
    remove_device_not_supporting_extension(
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    for (auto &information : m_physicalDevicesInformation)
        information.extensions.push_back(
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    m_synchronisation_2_enabled.setSynchronization2(true);
    m_features.setPNext(&m_synchronisation_2_enabled);
    return std::move(*this);
}

std::optional<PhysicalDevice> DeviceFinder::get() && noexcept {
    if (m_physicalDevicesInformation.empty())
        return {};
    return std::ranges::max_element(m_physicalDevicesInformation, std::less<>{},
                                    &PhysicalDeviceInformation::device)
        ->device;
}

void DeviceFinder::remove_device_not_supporting_extension(
    const char *extension) {
    auto need_to_remove =
        [extension](const PhysicalDeviceInformation &information) {
            return information.availableExtensions.find(extension) ==
                   information.availableExtensions.end();
        };
    std::erase_if(m_physicalDevicesInformation, need_to_remove);
}

Device DeviceFinder::build() && {
    if (m_physicalDevicesInformation.empty())
        throw DeviceNotFoundException{std::source_location::current()};

    auto information =
        *std::ranges::max_element(m_physicalDevicesInformation, std::less<>{},
                                  &PhysicalDeviceInformation::device);

    if (std::ranges::find(information.availableExtensions,
                          "VK_KHR_portability_subset") !=
        information.availableExtensions.end())
        information.extensions.push_back("VK_KHR_portability_subset");

    vk::DeviceCreateInfo info;
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;

    constexpr float priority = {1.0f};

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

    info.setPNext(&m_features);

    auto [result, device] =
        information.device.device().createDeviceUnique(info);

    if (result != vk::Result::eSuccess)
        throw DeviceCreationException{std::source_location::current()};

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

    return Device{std::move(device), information.device.device(),
                  std::move(queues), std::move(presentQueue)};
}

} // namespace vw
