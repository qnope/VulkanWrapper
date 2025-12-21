#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw {

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
