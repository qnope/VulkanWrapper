#include "VulkanWrapper/RayTracing/TopLevelAccelerationStructure.h"

#include "VulkanWrapper/Memory/AllocateBufferUtils.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::rt::as {

TopLevelAccelerationStructure::TopLevelAccelerationStructure(
    vk::UniqueAccelerationStructureKHR acceleration_structure,
    vk::DeviceAddress address, AccelerationStructureBuffer buffer)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(acceleration_structure))
    , m_device_address(address)
    , m_buffer(std::move(buffer)) {}

vk::DeviceAddress
TopLevelAccelerationStructure::device_address() const noexcept {
    return m_device_address;
}

TopLevelAccelerationStructureBuilder::TopLevelAccelerationStructureBuilder(
    std::shared_ptr<const Device> device,
    std::shared_ptr<const Allocator> allocator)
    : m_device(std::move(device))
    , m_allocator(std::move(allocator)) {}

TopLevelAccelerationStructureBuilder &TopLevelAccelerationStructureBuilder::
    add_bottom_level_acceleration_structure_address(
        vk::DeviceAddress address, const glm::mat4 &transform) {
    vk::AccelerationStructureInstanceKHR instance;

    // Convert glm::mat4 to vk::TransformMatrixKHR
    // Vulkan expects a 3x4 row-major transform matrix
    vk::TransformMatrixKHR transform_matrix;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            transform_matrix.matrix[row][col] = transform[col][row];
        }
    }

    instance.setTransform(transform_matrix);
    instance.setInstanceCustomIndex(static_cast<uint32_t>(m_instances.size()));
    instance.setMask(0xFF);
    instance.setInstanceShaderBindingTableRecordOffset(0);
    instance.setFlags(
        vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
    instance.setAccelerationStructureReference(address);

    m_instances.push_back(instance);
    return *this;
}

TopLevelAccelerationStructure
TopLevelAccelerationStructureBuilder::build(vk::CommandBuffer command_buffer) {
    // Create instance buffer
    auto instance_buffer =
        create_buffer<InstanceBuffer>(*m_allocator, m_instances.size());

    // Upload instances to buffer
    instance_buffer.write(m_instances, 0);

    // Build geometry info
    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    vk::AccelerationStructureGeometryInstancesDataKHR instances_data;
    instances_data.setArrayOfPointers(false);
    instances_data.setData(instance_buffer.device_address());
    geometry.geometry.setInstances(instances_data);

    vk::AccelerationStructureBuildGeometryInfoKHR build_info;
    build_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
    build_info.setFlags(
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    build_info.setGeometries(geometry);
    build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);

    uint32_t primitive_count = static_cast<uint32_t>(m_instances.size());

    // Query build sizes
    const auto build_sizes =
        m_device->handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, build_info,
            primitive_count);

    // Allocate acceleration structure buffer
    auto as_buffer = create_buffer<AccelerationStructureBuffer>(
        *m_allocator, build_sizes.accelerationStructureSize);

    // Create acceleration structure
    vk::AccelerationStructureCreateInfoKHR create_info;
    create_info.setBuffer(as_buffer.handle());
    create_info.setOffset(0);
    create_info.setSize(build_sizes.accelerationStructureSize);
    create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel);

    auto acceleration_structure =
        m_device->handle()
            .createAccelerationStructureKHRUnique(create_info)
            .value;

    // Get device address
    vk::AccelerationStructureDeviceAddressInfoKHR address_info;
    address_info.setAccelerationStructure(*acceleration_structure);
    auto address =
        m_device->handle().getAccelerationStructureAddressKHR(address_info);

    // Allocate scratch buffer
    auto scratch_buffer = create_buffer<ScratchBuffer>(
        *m_allocator, build_sizes.buildScratchSize);

    // Build acceleration structure
    build_info.setDstAccelerationStructure(*acceleration_structure);
    build_info.setScratchData(scratch_buffer.device_address());

    vk::AccelerationStructureBuildRangeInfoKHR range_info;
    range_info.setPrimitiveCount(primitive_count);
    range_info.setPrimitiveOffset(0);
    range_info.setFirstVertex(0);
    range_info.setTransformOffset(0);

    const auto *p_range = &range_info;
    command_buffer.buildAccelerationStructuresKHR(build_info, p_range);

    return TopLevelAccelerationStructure(std::move(acceleration_structure),
                                         address, std::move(as_buffer));
}

} // namespace vw::rt::as
