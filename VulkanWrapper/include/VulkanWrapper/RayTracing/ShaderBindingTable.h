#pragma once

#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

namespace vw::rt {

constexpr uint64_t ShaderBindingTableUserDataSize = 256;

constexpr uint64_t ShaderBindingTableHandleRecordSize =
    ShaderBindingTableHandleSizeAlignment + ShaderBindingTableUserDataSize;

constexpr auto ShaderBindingTableUsage =
    VkBufferUsageFlags(vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress);

struct ShaderBindingTableUserData {
    std::array<std::byte, ShaderBindingTableUserDataSize> data{};
};

struct alignas(ShaderBindingTableHandleSizeAlignment) ShaderBindingTableRecord {
    ShaderBindingTableHandle handle;
    ShaderBindingTableUserData user_data{};
};

class ShaderBindingTable {
  public:
  private:
    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_buffer;
};

} // namespace vw::rt
