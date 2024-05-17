#include "3DRenderer/Core/Vulkan/PhysicalDevice.h"

namespace r3d {
namespace {
PhysicalDeviceType
convertPhysicalDeviceType(vk::PhysicalDeviceType physicalDeviceType) {
    switch (physicalDeviceType) {
    case vk::PhysicalDeviceType::eDiscreteGpu:
        return PhysicalDeviceType::DiscreteGpu;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        return PhysicalDeviceType::IntegratedGpu;
    case vk::PhysicalDeviceType::eCpu:
        return PhysicalDeviceType::Cpu;
    default:
        return PhysicalDeviceType::Other;
    }
}
} // namespace

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice device) noexcept
    : m_type(convertPhysicalDeviceType(device.getProperties().deviceType))
    , m_name{device.getProperties().deviceName}
    , m_physicalDevice{device} {}

std::vector<vk::QueueFamilyProperties>
PhysicalDevice::queueFamilyProperties() const noexcept {
    return m_physicalDevice.getQueueFamilyProperties();
}

std::vector<const char *> PhysicalDevice::extensions() const noexcept {
    auto extensions =
        m_physicalDevice.enumerateDeviceExtensionProperties().value;
    std::vector<const char *> result;
    for (auto &extension : extensions) {
        result.push_back(extension.extensionName);
    }
    return result;
}

vk::PhysicalDevice PhysicalDevice::device() const noexcept {
    return m_physicalDevice;
}

} // namespace r3d
