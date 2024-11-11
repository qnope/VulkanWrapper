#pragma once

namespace vw {

enum class PhysicalDeviceType { Other, Cpu, IntegratedGpu, DiscreteGpu };

class PhysicalDevice {
  public:
    PhysicalDevice(vk::PhysicalDevice physicalDevice) noexcept;

    std::vector<vk::QueueFamilyProperties>
    queueFamilyProperties() const noexcept;

    std::set<std::string> extensions() const noexcept;

    vk::PhysicalDevice device() const noexcept;

  private:
    PhysicalDeviceType m_type;
    std::string m_name;

    vk::PhysicalDevice m_physicalDevice;

    friend std::strong_ordering operator<=>(const PhysicalDevice &,
                                            const PhysicalDevice &) = default;
};
} // namespace vw
