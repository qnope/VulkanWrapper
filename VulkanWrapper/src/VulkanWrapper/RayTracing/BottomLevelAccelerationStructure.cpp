#include "VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h"
#include "VulkanWrapper/Model/Mesh.h"

#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Synchronization/Fence.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <vulkan/vulkan.hpp>

namespace vw::rt::as {

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    vk::UniqueAccelerationStructureKHR acceleration_structure,
    vk::DeviceAddress address)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(acceleration_structure))
    , m_device_address(address) {}

vk::DeviceAddress
BottomLevelAccelerationStructure::device_address() const noexcept {
    return m_device_address;
}

BottomLevelAccelerationStructureList::BottomLevelAccelerationStructureList(
    const Device &device, const Allocator &allocator)
    : m_acceleration_structure_buffer_list(allocator)
    , m_scratch_buffer_list(allocator)
    , m_command_pool(CommandPoolBuilder(device).build())
    , m_command_buffer(m_command_pool.allocate(1).front())
    , m_device(device) {
    std::ignore = m_command_buffer.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
}

BottomLevelAccelerationStructureList::AccelerationStructureBufferList::BufferInfo
BottomLevelAccelerationStructureList::allocate_acceleration_structure_buffer(
    vk::DeviceSize size) {
    return m_acceleration_structure_buffer_list.create_buffer(size);
}

BottomLevelAccelerationStructureList::ScratchBufferList::BufferInfo
BottomLevelAccelerationStructureList::allocate_scratch_buffer(
    vk::DeviceSize size) {
    return m_scratch_buffer_list.create_buffer(size);
}

BottomLevelAccelerationStructure &BottomLevelAccelerationStructureList::add(
    BottomLevelAccelerationStructure &&blas) {
    return m_all_bottom_level_acceleration_structure.emplace_back(
        std::move(blas));
}

std::vector<vk::DeviceAddress>
BottomLevelAccelerationStructureList::device_addresses() const {
    std::vector<vk::DeviceAddress> addresses;
    addresses.reserve(m_all_bottom_level_acceleration_structure.size());
    for (const auto &blas : m_all_bottom_level_acceleration_structure) {
        addresses.push_back(blas.device_address());
    }
    return addresses;
}

vk::CommandBuffer BottomLevelAccelerationStructureList::command_buffer() {
    return m_command_buffer;
}

void BottomLevelAccelerationStructureList::submit_and_wait() {
    std::ignore = m_command_buffer.end();
    auto &queue = const_cast<Device &>(m_device).graphicsQueue();
    queue.enqueue_command_buffer(m_command_buffer);
    queue.submit({}, {}, {});
    m_device.wait_idle();
}

BottomLevelAccelerationStructureBuilder::
    BottomLevelAccelerationStructureBuilder(const Device &device)
    : m_device(device) {}

BottomLevelAccelerationStructureBuilder &
BottomLevelAccelerationStructureBuilder::add_geometry(
    const vk::AccelerationStructureGeometryKHR &geometry,
    const vk::AccelerationStructureBuildRangeInfoKHR &offset) {
    m_geometries.push_back(geometry);
    m_ranges.push_back(offset);
    return *this;
}

BottomLevelAccelerationStructureBuilder &
BottomLevelAccelerationStructureBuilder::add_mesh(const Model::Mesh &mesh) {
    return add_geometry(mesh.acceleration_structure_geometry(),
                        mesh.acceleration_structure_range_info());
}

BottomLevelAccelerationStructure &
BottomLevelAccelerationStructureBuilder::build_into(
    BottomLevelAccelerationStructureList &list) {
    vk::AccelerationStructureBuildGeometryInfoKHR build_info;
    build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
    build_info.setFlags(
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    build_info.setGeometries(m_geometries);
    build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);

    std::vector<uint32_t> max_primitive_counts;
    max_primitive_counts.reserve(m_ranges.size());
    for (const auto &range : m_ranges) {
        max_primitive_counts.push_back(range.primitiveCount);
    }

    const auto build_sizes =
        m_device.handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, build_info,
            max_primitive_counts);

    auto buffer_info = list.allocate_acceleration_structure_buffer(
        build_sizes.accelerationStructureSize);

    vk::AccelerationStructureCreateInfoKHR create_info;
    create_info.setBuffer(buffer_info.buffer->handle());
    create_info.setOffset(buffer_info.offset);
    create_info.setSize(build_sizes.accelerationStructureSize);
    create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

    auto acceleration_structure =
        m_device.handle()
            .createAccelerationStructureKHRUnique(create_info)
            .value;

    vk::AccelerationStructureDeviceAddressInfoKHR address_info;
    address_info.setAccelerationStructure(*acceleration_structure);
    auto address =
        m_device.handle().getAccelerationStructureAddressKHR(address_info);

    auto scratch_buffer =
        list.allocate_scratch_buffer(build_sizes.buildScratchSize);

    build_info.setDstAccelerationStructure(*acceleration_structure);
    build_info.setScratchData(scratch_buffer.buffer->device_address() +
                              scratch_buffer.offset);

    auto command_buffer = list.command_buffer();

    const auto *p_ranges = m_ranges.data();
    command_buffer.buildAccelerationStructuresKHR(1, &build_info, &p_ranges);

    return list.add(BottomLevelAccelerationStructure(
        std::move(acceleration_structure), address));
}

} // namespace vw::rt::as
