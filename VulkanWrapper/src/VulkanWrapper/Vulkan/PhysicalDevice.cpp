#include "VulkanWrapper/Vulkan/PhysicalDevice.h"

namespace vw {
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

std::set<std::string> PhysicalDevice::extensions() const noexcept {
    auto extensions =
        m_physicalDevice.enumerateDeviceExtensionProperties().value;
    auto names = extensions |
                 std::views::transform(&vk::ExtensionProperties::extensionName);
    return {names.begin(), names.end()};
}

vk::PhysicalDevice PhysicalDevice::device() const noexcept {
    return m_physicalDevice;
}

} // namespace vw
