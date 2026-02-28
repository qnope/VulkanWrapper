module;
module vw.vulkan;
import std3rd;
import vulkan3rd;

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
    , m_version{ApiVersion(device.getProperties().apiVersion)}
    , m_name{device.getProperties().deviceName.data()}
    , m_physicalDevice{device} {}

std::vector<vk::QueueFamilyProperties>
PhysicalDevice::queueFamilyProperties() const noexcept {
    return m_physicalDevice.getQueueFamilyProperties();
}

std::set<std::string> PhysicalDevice::extensions() const noexcept {
    auto extensions =
        m_physicalDevice.enumerateDeviceExtensionProperties().value;
    auto step1 = std::ranges::views::transform(
        extensions, &vk::ExtensionProperties::extensionName);
    auto names = std::ranges::views::transform(
        step1, [](auto &x) -> std::string { return x.data(); });
    return {names.begin(), names.end()};
}

std::string_view PhysicalDevice::name() const noexcept { return m_name; }

vk::PhysicalDevice PhysicalDevice::device() const noexcept {
    return m_physicalDevice;
}

ApiVersion PhysicalDevice::api_version() const noexcept { return m_version; }

} // namespace vw
