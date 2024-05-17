#pragma once

#include "3DRenderer/3rd_party.h"

namespace r3d {

enum class PhysicalDeviceType { Other, Cpu, IntegratedGpu, DiscreteGpu };

class PhysicalDevice {
  public:
    PhysicalDevice(vk::PhysicalDevice physicalDevice) noexcept;

    std::vector<vk::QueueFamilyProperties>
    queueFamilyProperties() const noexcept;

    std::vector<const char *> extensions() const noexcept;

    vk::PhysicalDevice device() const noexcept;

  private:
    PhysicalDeviceType m_type;
    std::string m_name;

    vk::PhysicalDevice m_physicalDevice;

    friend std::strong_ordering operator<=>(const PhysicalDevice &,
                                            const PhysicalDevice &) = default;
};
} // namespace r3d
