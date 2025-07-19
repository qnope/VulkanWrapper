#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Model/Mesh.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw::AccelerationStructure {

// Type aliases for cleaner code
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

class BottomLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    BottomLevelAccelerationStructure(
        const Device &device, const Allocator &allocator,
        vk::UniqueAccelerationStructureKHR accelerationStructure,
        vk::DeviceAddress deviceAddress, AccelerationStructureBuffer buffer,
        vk::DeviceSize size);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept {
        return m_deviceAddress;
    }
    [[nodiscard]] const AccelerationStructureBuffer &buffer() const noexcept {
        return m_buffer;
    }
    [[nodiscard]] vk::DeviceSize size() const noexcept { return m_size; }

  private:
    vk::DeviceAddress m_deviceAddress;
    AccelerationStructureBuffer m_buffer;
    vk::DeviceSize m_size;
};

class TopLevelAccelerationStructure
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
    TopLevelAccelerationStructure(
        const Device &device, const Allocator &allocator,
        vk::UniqueAccelerationStructureKHR accelerationStructure,
        vk::DeviceAddress deviceAddress, AccelerationStructureBuffer buffer,
        vk::DeviceSize size);

    [[nodiscard]] vk::DeviceAddress device_address() const noexcept {
        return m_deviceAddress;
    }
    [[nodiscard]] const AccelerationStructureBuffer &buffer() const noexcept {
        return m_buffer;
    }
    [[nodiscard]] vk::DeviceSize size() const noexcept { return m_size; }

  private:
    vk::DeviceAddress m_deviceAddress;
    AccelerationStructureBuffer m_buffer;
    vk::DeviceSize m_size;
};

class BottomLevelAccelerationStructureBuilder {
  public:
    BottomLevelAccelerationStructureBuilder(Device &device,
                                            const Allocator &allocator);

    BottomLevelAccelerationStructureBuilder &
    add_geometry(const Model::Mesh &mesh);
    BottomLevelAccelerationStructureBuilder &&
    add_geometries(std::span<const Model::Mesh> meshes);

    BottomLevelAccelerationStructure build() &&;

  private:
    Device *m_device;
    const Allocator *m_allocator;
    std::vector<vk::AccelerationStructureGeometryKHR> m_geometries;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> m_range_info;
    std::vector<uint32_t> m_primitive_count;
};

class TopLevelAccelerationStructureBuilder {
  public:
    TopLevelAccelerationStructureBuilder(Device &device,
                                         const Allocator &allocator);

    TopLevelAccelerationStructureBuilder &&
    add_instance(const BottomLevelAccelerationStructure &blas,
                 const glm::mat4 &transform = glm::mat4(1.0f),
                 uint32_t instanceId = 0, uint32_t mask = 0xFF,
                 uint32_t hitGroupIndex = 0);

    TopLevelAccelerationStructureBuilder &
    add_instances(std::span<const BottomLevelAccelerationStructure> blases,
                  std::span<const glm::mat4> transforms = {});

    TopLevelAccelerationStructure build() &&;

  private:
    Device *m_device;
    const Allocator *m_allocator;
    std::vector<vk::AccelerationStructureInstanceKHR> m_instances;
};

} // namespace vw::AccelerationStructure
