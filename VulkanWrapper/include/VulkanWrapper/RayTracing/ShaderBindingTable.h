#pragma once

#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

namespace vw::rt {

constexpr uint64_t ShaderBindingTableUserDataSize = 256;

constexpr uint64_t ShaderBindingTableHandleRecordSize =
    ShaderBindingTableHandleSizeAlignment + ShaderBindingTableUserDataSize;

constexpr uint64_t MaximumRecordInShaderBindingTable = 4'096;

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
    ShaderBindingTable(const Allocator &allocator,
                       const ShaderBindingTableHandle &raygen_handle);

    void add_miss_record(const ShaderBindingTableHandle &handle);

    void add_hit_record(const ShaderBindingTableHandle &handle);

    [[nodiscard]] vk::StridedDeviceAddressRegionKHR raygen_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR miss_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR hit_region() const;

  private:
    const Allocator &m_allocator;

    uint64_t m_number_raygen_miss = 0;
    uint64_t m_number_hit = 0;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_ray_generation_and_miss_buffer;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_closest_hit_buffer;
};

} // namespace vw::rt
