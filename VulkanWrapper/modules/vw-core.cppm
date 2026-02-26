module;
#include "VulkanWrapper/3rd_party.h"
#include <vulkan/vulkan.hpp>

export module vw:core;

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

template <typename... Fs> struct overloaded : Fs... {
    using Fs::operator()...;
};

constexpr VkBufferUsageFlags2 VertexBufferUsage = VkBufferUsageFlags2{
    vk::BufferUsageFlagBits2::eVertexBuffer |
    vk::BufferUsageFlagBits2::eTransferDst |
    vk::BufferUsageFlagBits2::eShaderDeviceAddress |
    vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR};

constexpr VkBufferUsageFlags2 IndexBufferUsage = VkBufferUsageFlags2{
    vk::BufferUsageFlagBits2::eIndexBuffer |
    vk::BufferUsageFlagBits2::eTransferDst |
    vk::BufferUsageFlagBits2::eShaderDeviceAddress |
    vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR};

constexpr VkBufferUsageFlags2 UniformBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eUniformBuffer |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

constexpr VkBufferUsageFlags2 StagingBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eTransferSrc |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

constexpr VkBufferUsageFlags2 StorageBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eStorageBuffer |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

} // namespace vw
