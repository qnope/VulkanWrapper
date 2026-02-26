module;
#include "VulkanWrapper/3rd_party.h"

export module vw:vulkan;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;

export namespace vw {

// Forward declarations for types from other partitions
class Image;
class ImageView;
class Semaphore;

// ---- PhysicalDevice ----

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

// ---- Queue ----

class Device;
class DeviceFinder;
class Fence;
class Swapchain;

class Queue {
    friend class DeviceFinder;
    friend class Device;

  public:
    void enqueue_command_buffer(vk::CommandBuffer command_buffer);
    void
    enqueue_command_buffers(std::span<const vk::CommandBuffer> command_buffers);

    Fence submit(std::span<const vk::PipelineStageFlags> waitStage,
                 std::span<const vk::Semaphore> waitSemaphores,
                 std::span<const vk::Semaphore> signalSemaphores);

  private:
    Queue(vk::Queue queue, vk::QueueFlags type) noexcept;

    std::vector<vk::CommandBuffer> m_command_buffers;

    vk::Device m_device;
    vk::Queue m_queue;
    vk::QueueFlags m_queueFlags;
};

// ---- PresentQueue ----

class PresentQueue {
    friend class DeviceFinder;

  public:
    void present(const Swapchain &swapchain, uint32_t imageIndex,
                 const Semaphore &waitSemaphore) const;

  private:
    PresentQueue(vk::Queue queue) noexcept;

    vk::Queue m_queue;
};

// ---- Device ----

struct DeviceImpl;

class Device {
    friend class DeviceFinder;

  public:
    Queue &graphicsQueue();
    [[nodiscard]] const PresentQueue &presentQueue() const;
    void wait_idle() const;
    [[nodiscard]] vk::PhysicalDevice physical_device() const;
    [[nodiscard]] vk::Device handle() const;

    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    Device(Device &&) = delete;
    Device &operator=(Device &&) = delete;

  private:
    Device(vk::UniqueDevice device, vk::PhysicalDevice physicalDevice,
           std::vector<Queue> queues,
           std::optional<PresentQueue> presentQueue) noexcept;

    std::shared_ptr<DeviceImpl> m_impl;
};

// ---- Instance ----

class Instance {
    friend class InstanceBuilder;

  public:
    Instance(const Instance &) = delete;
    Instance(Instance &&) = delete;

    Instance &operator=(Instance &&) = delete;
    Instance &operator=(const Instance &) = delete;

    [[nodiscard]] vk::Instance handle() const noexcept;

    [[nodiscard]] DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance,
             vk::UniqueDebugUtilsMessengerEXT debugMessenger,
             std::span<const char *> extensions,
             ApiVersion apiVersion) noexcept;

    struct Impl {
        vk::UniqueInstance instance;
        vk::UniqueDebugUtilsMessengerEXT debugMessenger;
        std::vector<const char *> extensions;
        ApiVersion version;

        Impl(vk::UniqueInstance inst,
             vk::UniqueDebugUtilsMessengerEXT debugMsgr,
             std::span<const char *> exts, ApiVersion apiVersion) noexcept;

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;
        Impl(Impl &&) = delete;
        Impl &operator=(Impl &&) = delete;
    };

    std::shared_ptr<Impl> m_impl;
};

class InstanceBuilder {
  public:
    InstanceBuilder &addPortability();
    InstanceBuilder &addExtension(const char *extension);
    InstanceBuilder &addExtensions(std::span<const char *const> extensions);
    InstanceBuilder &setDebug();
    InstanceBuilder &setApiVersion(ApiVersion version);

    std::shared_ptr<Instance> build();

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = false;
    ApiVersion m_version = ApiVersion::e10;
};

// ---- DeviceFinder ----

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

// ---- Swapchain ----

class Swapchain : public ObjectWithUniqueHandle<vk::UniqueSwapchainKHR> {
  public:
    Swapchain(std::shared_ptr<const Device> device,
              vk::UniqueSwapchainKHR swapchain, vk::Format format, Width width,
              Height height);

    [[nodiscard]] Width width() const noexcept;
    [[nodiscard]] Height height() const noexcept;
    [[nodiscard]] vk::Format format() const noexcept;

    [[nodiscard]] std::span<const std::shared_ptr<const Image>>
    images() const noexcept;

    [[nodiscard]] std::span<const std::shared_ptr<const ImageView>>
    image_views() const noexcept;

    [[nodiscard]] int number_images() const noexcept;

    [[nodiscard]] uint64_t acquire_next_image(const Semaphore &semaphore) const;

    void present(uint32_t index, const Semaphore &waitSemaphore) const;

  private:
    std::shared_ptr<const Device> m_device;
    vk::Format m_format;
    std::vector<std::shared_ptr<const Image>> m_images;
    std::vector<std::shared_ptr<const ImageView>> m_image_views;
    Width m_width;
    Height m_height;
};

class SwapchainBuilder {
  public:
    SwapchainBuilder(std::shared_ptr<const Device> device,
                     vk::SurfaceKHR surface, Width width,
                     Height height) noexcept;
    SwapchainBuilder &with_old_swapchain(vk::SwapchainKHR old);
    Swapchain build();

  private:
    std::shared_ptr<const Device> m_device;
    Width m_width;
    Height m_height;

    vk::SwapchainCreateInfoKHR m_info;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eMailbox;
};

// ---- Surface ----

class Surface : public ObjectWithUniqueHandle<vk::UniqueSurfaceKHR> {
  public:
    Surface(vk::UniqueSurfaceKHR surface) noexcept;
};

} // namespace vw
