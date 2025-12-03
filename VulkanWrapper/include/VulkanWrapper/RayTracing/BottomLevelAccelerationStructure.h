#pragma once
#include "VulkanWrapper/3rd_party.h"

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
    BottomLevelAccelerationStructureList(std::shared_ptr<const Device> device,
                                         std::shared_ptr<const Allocator> allocator);

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

    AccelerationStructureBufferList::BufferInfo
    allocate_acceleration_structure_buffer(vk::DeviceSize size);
    ScratchBufferList::BufferInfo allocate_scratch_buffer(vk::DeviceSize size);

    BottomLevelAccelerationStructure &add(BottomLevelAccelerationStructure &&blas);
    [[nodiscard]] std::vector<vk::DeviceAddress> device_addresses() const;

    vk::CommandBuffer command_buffer();
    void submit_and_wait();

  private:
    AccelerationStructureBufferList m_acceleration_structure_buffer_list;
    ScratchBufferList m_scratch_buffer_list;
    std::vector<BottomLevelAccelerationStructure>
        m_all_bottom_level_acceleration_structure;

    CommandPool m_command_pool;
    vk::CommandBuffer m_command_buffer;
    std::shared_ptr<const Device> m_device;
};

class BottomLevelAccelerationStructureBuilder {
  public:
    BottomLevelAccelerationStructureBuilder(std::shared_ptr<const Device> device);

    BottomLevelAccelerationStructureBuilder &
    add_geometry(const vk::AccelerationStructureGeometryKHR &geometry,
                 const vk::AccelerationStructureBuildRangeInfoKHR &offset);

    BottomLevelAccelerationStructureBuilder &add_mesh(const Model::Mesh &mesh);

    BottomLevelAccelerationStructure &
    build_into(BottomLevelAccelerationStructureList &list);

  private:
    std::shared_ptr<const Device> m_device;
    std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_ranges;
};

} // namespace vw::rt::as
