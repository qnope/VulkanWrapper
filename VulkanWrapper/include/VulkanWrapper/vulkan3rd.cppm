module;

namespace vk::detail {
class DispatchLoaderDynamic;
}
namespace vw {
const vk::detail::DispatchLoaderDynamic &DefaultDispatcher();
}

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

export module vulkan3rd;

export namespace vk {
using vk::AccelerationStructureBuildGeometryInfoKHR;
using vk::AccelerationStructureBuildRangeInfoKHR;
using vk::AccelerationStructureBuildSizesInfoKHR;
using vk::AccelerationStructureBuildTypeKHR;
using vk::AccelerationStructureCreateInfoKHR;
using vk::AccelerationStructureDeviceAddressInfoKHR;
using vk::AccelerationStructureGeometryDataKHR;
using vk::AccelerationStructureGeometryInstancesDataKHR;
using vk::AccelerationStructureGeometryKHR;
using vk::AccelerationStructureGeometryTrianglesDataKHR;
using vk::AccelerationStructureInstanceKHR;
using vk::AccelerationStructureKHR;
using vk::AccelerationStructureTypeKHR;
using vk::AccessFlagBits2;
using vk::AccessFlags2;
using vk::ApplicationInfo;
using vk::AttachmentLoadOp;
using vk::AttachmentStoreOp;
using vk::BlendFactor;
using vk::BlendOp;
using vk::Bool32;
using vk::Buffer;
using vk::BufferCopy;
using vk::BufferCreateInfo;
using vk::BufferDeviceAddressInfo;
using vk::BufferDeviceAddressInfoKHR;
using vk::BufferImageCopy;
using vk::BufferMemoryBarrier2;
using vk::BufferUsageFlagBits;
using vk::BufferUsageFlagBits2;
using vk::BufferUsageFlags;
using vk::BufferUsageFlags2;
using vk::BuildAccelerationStructureFlagBitsKHR;
using vk::BuildAccelerationStructureFlagsKHR;
using vk::BuildAccelerationStructureModeKHR;
using vk::ClearColorValue;
using vk::ClearDepthStencilValue;
using vk::ClearValue;
using vk::ColorComponentFlagBits;
using vk::ColorComponentFlags;
using vk::ColorSpaceKHR;
using vk::CommandBuffer;
using vk::CommandBufferAllocateInfo;
using vk::CommandBufferBeginInfo;
using vk::CommandBufferLevel;
using vk::CommandBufferUsageFlagBits;
using vk::CommandPoolCreateFlagBits;
using vk::CommandPoolCreateFlags;
using vk::CommandPoolCreateInfo;
using vk::CompareOp;
using vk::ComponentMapping;
using vk::ComponentSwizzle;
using vk::CompositeAlphaFlagBitsKHR;
using vk::ComputePipelineCreateInfo;
using vk::CullModeFlagBits;
using vk::CullModeFlags;
using vk::DebugUtilsMessageSeverityFlagBitsEXT;
using vk::DebugUtilsMessageTypeFlagBitsEXT;
using vk::DebugUtilsMessageTypeFlagsEXT;
using vk::DebugUtilsMessengerCallbackDataEXT;
using vk::DebugUtilsMessengerCreateInfoEXT;
using vk::DeferredOperationKHR;
using vk::DependencyInfo;
using vk::DescriptorBindingFlagBits;
using vk::DescriptorBindingFlags;
using vk::DescriptorBufferInfo;
using vk::DescriptorImageInfo;
using vk::DescriptorPool;
using vk::DescriptorPoolCreateFlagBits;
using vk::DescriptorPoolCreateFlags;
using vk::DescriptorPoolCreateInfo;
using vk::DescriptorPoolSize;
using vk::DescriptorSet;
using vk::DescriptorSetAllocateInfo;
using vk::DescriptorSetLayout;
using vk::DescriptorSetLayoutBinding;
using vk::DescriptorSetLayoutBindingFlagsCreateInfo;
using vk::DescriptorSetLayoutCreateFlagBits;
using vk::DescriptorSetLayoutCreateInfo;
using vk::DescriptorType;
using vk::Device;
using vk::DeviceAddress;
using vk::DeviceCreateInfo;
using vk::DeviceQueueCreateInfo;
using vk::DeviceSize;
using vk::DynamicState;
using vk::ExtensionProperties;
using vk::Extent2D;
using vk::Extent3D;
using vk::False;
using vk::Fence;
using vk::FenceCreateFlagBits;
using vk::FenceCreateInfo;
using vk::Filter;
using vk::Format;
using vk::FrontFace;
using vk::GeometryFlagBitsKHR;
using vk::GeometryFlagsKHR;
using vk::GeometryInstanceFlagBitsKHR;
using vk::GeometryInstanceFlagsKHR;
using vk::GeometryTypeKHR;
using vk::GraphicsPipelineCreateInfo;
using vk::Image;
using vk::ImageAspectFlagBits;
using vk::ImageAspectFlags;
using vk::ImageBlit;
using vk::ImageCreateInfo;
using vk::ImageLayout;
using vk::ImageMemoryBarrier2;
using vk::ImageSubresourceLayers;
using vk::ImageSubresourceRange;
using vk::ImageType;
using vk::ImageUsageFlagBits;
using vk::ImageUsageFlags;
using vk::ImageView;
using vk::ImageViewCreateInfo;
using vk::ImageViewType;
using vk::IndexType;
using vk::Instance;
using vk::InstanceCreateFlags;
using vk::InstanceCreateInfo;
using vk::MemoryBarrier2;
using vk::Offset2D;
using vk::Offset3D;
using vk::PhysicalDevice;
using vk::PhysicalDeviceAccelerationStructureFeaturesKHR;
using vk::PhysicalDeviceDynamicRenderingFeatures;
using vk::PhysicalDeviceFeatures2;
using vk::PhysicalDeviceProperties2;
using vk::PhysicalDeviceRayQueryFeaturesKHR;
using vk::PhysicalDeviceRayTracingPipelineFeaturesKHR;
using vk::PhysicalDeviceRayTracingPipelinePropertiesKHR;
using vk::PhysicalDeviceSynchronization2Features;
using vk::PhysicalDeviceType;
using vk::PhysicalDeviceVulkan12Features;
using vk::PipelineBindPoint;
using vk::PipelineCache;
using vk::PipelineColorBlendAttachmentState;
using vk::PipelineColorBlendStateCreateInfo;
using vk::PipelineDepthStencilStateCreateInfo;
using vk::PipelineDynamicStateCreateInfo;
using vk::PipelineInputAssemblyStateCreateInfo;
using vk::PipelineLayout;
using vk::PipelineLayoutCreateInfo;
using vk::PipelineMultisampleStateCreateInfo;
using vk::PipelineRasterizationStateCreateInfo;
using vk::PipelineRenderingCreateInfo;
using vk::PipelineShaderStageCreateInfo;
using vk::PipelineStageFlagBits;
using vk::PipelineStageFlagBits2;
using vk::PipelineStageFlags;
using vk::PipelineStageFlags2;
using vk::PipelineVertexInputStateCreateInfo;
using vk::PipelineViewportStateCreateInfo;
using vk::PolygonMode;
using vk::PresentInfoKHR;
using vk::PresentModeKHR;
using vk::PrimitiveTopology;
using vk::PushConstantRange;
using vk::Queue;
using vk::QueueFamilyProperties;
using vk::QueueFlagBits;
using vk::QueueFlags;
using vk::RayTracingPipelineCreateInfoKHR;
using vk::RayTracingShaderGroupCreateInfoKHR;
using vk::RayTracingShaderGroupTypeKHR;
using vk::Rect2D;
using vk::RenderingAttachmentInfo;
using vk::RenderingInfo;
using vk::Result;
using vk::ResultValue;
using vk::SampleCountFlagBits;
using vk::Sampler;
using vk::SamplerAddressMode;
using vk::SamplerCreateInfo;
using vk::SamplerMipmapMode;
using vk::Semaphore;
using vk::SemaphoreCreateInfo;
using vk::ShaderModule;
using vk::ShaderModuleCreateInfo;
using vk::ShaderStageFlagBits;
using vk::ShaderStageFlags;
using vk::SharingMode;
using vk::StridedDeviceAddressRegionKHR;
using vk::StructureChain;
using vk::SubmitInfo;
using vk::SubmitInfo2;
using vk::SurfaceCapabilitiesKHR;
using vk::SurfaceFormatKHR;
using vk::SurfaceKHR;
using vk::SwapchainCreateInfoKHR;
using vk::SwapchainKHR;
using vk::to_string;
using vk::TransformMatrixKHR;
using vk::True;
using vk::UniqueAccelerationStructureKHR;
using vk::UniqueCommandPool;
using vk::UniqueDebugUtilsMessengerEXT;
using vk::UniqueDescriptorPool;
using vk::UniqueDescriptorSetLayout;
using vk::UniqueDevice;
using vk::UniqueFence;
using vk::UniqueImageView;
using vk::UniqueInstance;
using vk::UniquePipeline;
using vk::UniquePipelineLayout;
using vk::UniqueSampler;
using vk::UniqueSemaphore;
using vk::UniqueShaderModule;
using vk::UniqueSurfaceKHR;
using vk::UniqueSwapchainKHR;
using vk::VertexInputAttributeDescription;
using vk::VertexInputBindingDescription;
using vk::VertexInputRate;
using vk::Viewport;
using vk::WriteDescriptorSet;
using vk::WriteDescriptorSetAccelerationStructureKHR;
using vk::operator|;
using vk::operator&;
using vk::operator==;
using vk::operator!=;
using vk::operator<=>;
using vk::operator~;

} // namespace vk

// Vulkan C types
export using ::VkAccelerationStructureKHR;
export using ::VkBuffer;
export using ::VkBufferUsageFlags;
export using ::VkBufferUsageFlags2;
export using ::VkDeviceSize;
export using ::VkImage;
export using ::VkResult;
export using ::VkBufferUsageFlagBits;
export using ::VkImageCreateInfo;
export using ::VkBufferCreateInfo;
export using ::VkSurfaceKHR;
// VK_NULL_HANDLE is a macro (#define VK_NULL_HANDLE 0), export as constexpr
export constexpr auto vw_VK_NULL_HANDLE = VK_NULL_HANDLE;
export using ::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
export using ::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
export using ::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
export using ::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
export using ::VK_SUCCESS;
export using ::VK_ERROR_OUT_OF_DEVICE_MEMORY;
export using ::VK_ERROR_OUT_OF_HOST_MEMORY;
export using ::VK_ERROR_UNKNOWN;
export using ::int32_t;
export using ::uint32_t;
export using ::uint64_t;
export using ::size_t;

// Vulkan macro constants (macros cannot cross module boundaries)
export constexpr auto vw_VK_API_VERSION_1_0 = VK_API_VERSION_1_0;
export constexpr auto vw_VK_API_VERSION_1_1 = VK_API_VERSION_1_1;
export constexpr auto vw_VK_API_VERSION_1_2 = VK_API_VERSION_1_2;
export constexpr auto vw_VK_API_VERSION_1_3 = VK_API_VERSION_1_3;
export constexpr uint32_t vw_VK_API_VERSION_MAJOR(uint32_t v) {
    return VK_API_VERSION_MAJOR(v);
}
export constexpr uint32_t vw_VK_API_VERSION_MINOR(uint32_t v) {
    return VK_API_VERSION_MINOR(v);
}

export namespace vw {
enum ApiVersion {
    e10 = vk::ApiVersion10,
    e11 = vk::ApiVersion11,
    e12 = vk::ApiVersion12,
    e13 = vk::ApiVersion13
};

enum class Width {};
enum class Height {};
enum class Depth {};
enum class MipLevel {};
} // namespace vw
