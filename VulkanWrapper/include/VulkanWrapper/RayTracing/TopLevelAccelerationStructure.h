#pragma once

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
        vk::DeviceAddress address,
        std::unique_ptr<AccelerationStructureBuffer> buffer,
        std::unique_ptr<InstanceBuffer> instance_buffer,
        std::unique_ptr<ScratchBuffer> scratch_buffer);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept;

  private:
    vk::DeviceAddress m_device_address;
    std::unique_ptr<AccelerationStructureBuffer> m_buffer;
    std::unique_ptr<InstanceBuffer> m_instance_buffer;
    std::unique_ptr<ScratchBuffer> m_scratch_buffer;
};

class TopLevelAccelerationStructureBuilder {
  public:
    TopLevelAccelerationStructureBuilder(const Device &device);

    TopLevelAccelerationStructureBuilder &
    add_bottom_level_acceleration_structure_address(vk::DeviceAddress address,
                                                    const glm::mat4 &transform);

    TopLevelAccelerationStructure build(const Allocator &allocator,
                                        vk::CommandBuffer command_buffer);

  private:
    const Device &m_device;
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances;
};

} // namespace vw::rt::as
