#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

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

using InstanceBuffer = Buffer<
    vk::AccelerationStructureInstanceKHR, true,
    VkBufferUsageFlags(
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress)>;

class TopLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    TopLevelAccelerationStructure(
        vk::UniqueAccelerationStructureKHR acceleration_structure,
        vk::DeviceAddress address, AccelerationStructureBuffer buffer);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

  private:
    vk::DeviceAddress m_device_address;
    AccelerationStructureBuffer m_buffer;
};

class TopLevelAccelerationStructureBuilder {
  public:
    TopLevelAccelerationStructureBuilder(const Device &device,
                                         const Allocator &allocator);

    TopLevelAccelerationStructureBuilder &
    add_bottom_level_acceleration_structure_address(vk::DeviceAddress address,
                                                    const glm::mat4 &transform);

    TopLevelAccelerationStructure build(vk::CommandBuffer command_buffer);

  private:
    const Device &m_device;
    const Allocator &m_allocator;
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances;
};

} // namespace vw::rt::as
