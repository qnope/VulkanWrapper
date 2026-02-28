export module vw.vulkan:physical_device;
import std3rd;
import vulkan3rd;

export namespace vw {

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

    friend std::strong_ordering operator<=>(const PhysicalDevice &,
                                            const PhysicalDevice &) = default;

  private:
    PhysicalDeviceType m_type;
    ApiVersion m_version;
    std::string m_name;

    vk::PhysicalDevice m_physicalDevice;
};
} // namespace vw
