#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/RayTracing/RayTracingPipeline.h"

namespace vw::rt {

constexpr uint64_t ShaderBindingTableHandleRecordSize = 256;

constexpr uint64_t MaximumRecordInShaderBindingTable = 4'096;

constexpr auto ShaderBindingTableUsage =
    VkBufferUsageFlags(vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress);

struct alignas(ShaderBindingTableHandleSizeAlignment) ShaderBindingTableRecord {
    ShaderBindingTableRecord(std::span<const std::byte> handle) {
        std::ranges::copy(handle, data.begin());
    }

    ShaderBindingTableRecord(std::span<const std::byte> handle,
                             const auto &object) {
        static_assert(sizeof(object) + ShaderBindingTableHandleSizeAlignment <
                      ShaderBindingTableHandleRecordSize);
        std::ranges::copy(handle, data.begin());
        std::memcpy(data.data() + handle.size(), &object, sizeof(object));
    }

    std::array<std::byte, ShaderBindingTableHandleRecordSize> data{};
};

class ShaderBindingTable {
  public:
    ShaderBindingTable(std::shared_ptr<const Allocator> allocator,
                       const ShaderBindingTableHandle &raygen_handle);

    void add_miss_record(const ShaderBindingTableHandle &handle,
                         const auto &...object) {
        ShaderBindingTableRecord record{handle, object...};
        m_sbt_ray_generation_and_miss_buffer.write(record,
                                                   m_number_raygen_miss++);
    }

    void add_hit_record(const ShaderBindingTableHandle &handle,
                        const auto &...object) {
        ShaderBindingTableRecord record{handle, object...};
        m_sbt_closest_hit_buffer.write(record, m_number_hit++);
    }

    [[nodiscard]] vk::StridedDeviceAddressRegionKHR raygen_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR miss_region() const;
    [[nodiscard]] vk::StridedDeviceAddressRegionKHR hit_region() const;

  private:
    std::shared_ptr<const Allocator> m_allocator;

    uint64_t m_number_raygen_miss = 0;
    uint64_t m_number_hit = 0;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_ray_generation_and_miss_buffer;

    Buffer<ShaderBindingTableRecord, true, ShaderBindingTableUsage>
        m_sbt_closest_hit_buffer;
};

} // namespace vw::rt
