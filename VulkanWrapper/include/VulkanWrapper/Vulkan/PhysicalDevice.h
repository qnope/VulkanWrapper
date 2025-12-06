#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw {

enum class PhysicalDeviceType { Other, Cpu, IntegratedGpu, DiscreteGpu };

class PhysicalDevice {
  public:
    PhysicalDevice(vk::PhysicalDevice physicalDevice) noexcept;

    [[nodiscard]] std::vector<vk::QueueFamilyProperties>
    queueFamilyProperties() const noexcept;

    [[nodiscard]] std::set<std::string> extensions() const noexcept;
    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] vk::PhysicalDevice device() const noexcept;
    [[nodiscard]] ApiVersion api_version() const noexcept;

  private:
    PhysicalDeviceType m_type;
    ApiVersion m_version;
    std::string m_name;

    vk::PhysicalDevice m_physicalDevice;

    friend std::strong_ordering operator<=>(const PhysicalDevice &,
                                            const PhysicalDevice &) = default;
};
} // namespace vw
