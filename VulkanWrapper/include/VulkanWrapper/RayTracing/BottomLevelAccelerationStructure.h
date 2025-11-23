#pragma once

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/BufferList.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include "VulkanWrapper/fwd.h"

namespace vw::rt::as {

using AccelerationStructureBuffer =
    Buffer<std::byte, false,
           VkBufferUsageFlags(
               vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
               vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

using ScratchBuffer =
    Buffer<std::byte, false,
           VkBufferUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer |
                              vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

class BottomLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    BottomLevelAccelerationStructure(
        vk::UniqueAccelerationStructureKHR acceleration_structure,
        vk::DeviceAddress address);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

  private:
    vk::DeviceAddress m_device_address;
};

class BottomLevelAccelerationStructureList {
  public:
    BottomLevelAccelerationStructureList(const Device &device,
                                         const Allocator &allocator);

    using AccelerationStructureBufferList =
        BufferList<std::byte, false,
                   VkBufferUsageFlags(vk::BufferUsageFlagBits::
                                          eAccelerationStructureStorageKHR |
                                      vk::BufferUsageFlagBits::
                                          eShaderDeviceAddress)>;

    using ScratchBufferList =
        BufferList<std::byte, false,
                   VkBufferUsageFlags(
                       vk::BufferUsageFlagBits::eStorageBuffer |
                       vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

    AccelerationStructureBufferList &acceleration_structure_buffer_list();
    ScratchBufferList &scratch_buffer_list();

    void add(BottomLevelAccelerationStructure &&blas);

    vk::CommandBuffer allocate_command_buffer();
    void submit_and_wait(vk::CommandBuffer command_buffer);

  private:
    AccelerationStructureBufferList m_acceleration_structure_buffer_list;
    ScratchBufferList m_scratch_buffer_list;
    std::vector<BottomLevelAccelerationStructure>
        m_all_bottom_level_acceleration_structure;

    CommandPool m_command_pool;
    const Device &m_device;
};

class BottomLevelAccelerationStructureBuilder {
  public:
    BottomLevelAccelerationStructureBuilder(const Device &device);

    BottomLevelAccelerationStructureBuilder &
    add_geometry(const vk::AccelerationStructureGeometryKHR &geometry,
                 const vk::AccelerationStructureBuildRangeInfoKHR &offset);

    BottomLevelAccelerationStructure
    build(BottomLevelAccelerationStructureList &list);

  private:
    const Device &m_device;
    std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_ranges;
};

} // namespace vw::rt::as
