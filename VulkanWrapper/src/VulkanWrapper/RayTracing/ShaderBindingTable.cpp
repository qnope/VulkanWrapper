#include "VulkanWrapper/RayTracing/ShaderBindingTable.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw::rt {

ShaderBindingTable::ShaderBindingTable(
    const Allocator &allocator, const ShaderBindingTableHandle &raygen_handle)
    : m_allocator{allocator}
    , m_sbt_ray_generation_and_miss_buffer(
          allocator.create_buffer<ShaderBindingTableRecord, true,
                                  ShaderBindingTableUsage>(
              MaximumRecordInShaderBindingTable))
    , m_sbt_closest_hit_buffer(
          allocator.create_buffer<ShaderBindingTableRecord, true,
                                  ShaderBindingTableUsage>(
              MaximumRecordInShaderBindingTable)) {
    add_miss_record(raygen_handle);
}

vk::StridedDeviceAddressRegionKHR ShaderBindingTable::raygen_region() const {
    return vk::StridedDeviceAddressRegionKHR()
        .setSize(ShaderBindingTableHandleRecordSize)
        .setDeviceAddress(m_sbt_ray_generation_and_miss_buffer.device_address())
        .setStride(ShaderBindingTableHandleRecordSize);
}

vk::StridedDeviceAddressRegionKHR ShaderBindingTable::miss_region() const {
    return vk::StridedDeviceAddressRegionKHR()
        .setSize(ShaderBindingTableHandleRecordSize *
                 (m_number_raygen_miss - 1))
        .setDeviceAddress(
            m_sbt_ray_generation_and_miss_buffer.device_address() +
            ShaderBindingTableHandleRecordSize)
        .setStride(ShaderBindingTableHandleRecordSize);
}

vk::StridedDeviceAddressRegionKHR ShaderBindingTable::hit_region() const {
    return vk::StridedDeviceAddressRegionKHR()
        .setSize(ShaderBindingTableHandleRecordSize * m_number_hit)
        .setDeviceAddress(m_sbt_closest_hit_buffer.device_address())
        .setStride(ShaderBindingTableHandleRecordSize);
}

} // namespace vw::rt
