#include "VulkanWrapper/RayTracing/BottomLevelAccelerationStructure.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"
#include "VulkanWrapper/Vulkan/Queue.h"
#include <vulkan/vulkan.hpp>

namespace vw::rt::as {

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    vk::UniqueAccelerationStructureKHR acceleration_structure,
    vk::DeviceAddress address)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(acceleration_structure)),
      m_device_address(address) {}

vk::DeviceAddress
BottomLevelAccelerationStructure::device_address() const noexcept {
    return m_device_address;
}

BottomLevelAccelerationStructureList::BottomLevelAccelerationStructureList(
    const Device &device, const Allocator &allocator)
    : m_acceleration_structure_buffer_list(allocator),
      m_scratch_buffer_list(allocator),
      m_command_pool(CommandPoolBuilder(device).build()), m_device(device) {}

BottomLevelAccelerationStructureList::AccelerationStructureBufferList &
BottomLevelAccelerationStructureList::acceleration_structure_buffer_list() {
    return m_acceleration_structure_buffer_list;
}

BottomLevelAccelerationStructureList::ScratchBufferList &
BottomLevelAccelerationStructureList::scratch_buffer_list() {
    return m_scratch_buffer_list;
}

void BottomLevelAccelerationStructureList::add(
    BottomLevelAccelerationStructure &&blas) {
    m_all_bottom_level_acceleration_structure.emplace_back(std::move(blas));
}

vk::CommandBuffer
BottomLevelAccelerationStructureList::allocate_command_buffer() {
    return m_command_pool.allocate(1).front();
}

void BottomLevelAccelerationStructureList::submit_and_wait(
    vk::CommandBuffer command_buffer) {
    auto &queue = const_cast<Device &>(m_device).graphicsQueue();
    queue.enqueue_command_buffer(command_buffer);
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

BottomLevelAccelerationStructure BottomLevelAccelerationStructureBuilder::build(
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

    auto buffer_info = list.acceleration_structure_buffer_list().create_buffer(
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
        list.scratch_buffer_list().create_buffer(build_sizes.buildScratchSize);

    build_info.setDstAccelerationStructure(*acceleration_structure);
    build_info.setScratchData(scratch_buffer.buffer->device_address() +
                              scratch_buffer.offset);

    auto command_buffer = list.allocate_command_buffer();

    command_buffer.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    const auto *p_ranges = m_ranges.data();
    command_buffer.buildAccelerationStructuresKHR(1, &build_info, &p_ranges);

    command_buffer.end();

    list.submit_and_wait(command_buffer);

    return BottomLevelAccelerationStructure(std::move(acceleration_structure),
                                            address);
}

} // namespace vw::rt::as
