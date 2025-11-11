#include "VulkanWrapper/AccelerationStructure/AccelerationStructure.h"

#include "VulkanWrapper/Command/CommandBuffer.h"
#include "VulkanWrapper/Command/CommandPool.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw::AccelerationStructure {

using AccelerationStructureCreationException =
    TaggedException<struct AccelerationStructureCreationTag>;

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
    const Device &device, const Allocator &allocator,
    vk::UniqueAccelerationStructureKHR accelerationStructure,
    AccelerationStructureBuffer buffer, vk::DeviceSize size)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(accelerationStructure))
    , m_device(&device)
    , m_buffer(std::move(buffer))
    , m_size(size) {}

vk::DeviceAddress
BottomLevelAccelerationStructure::device_address() const noexcept {
    // Get device address
    const auto addressInfo = vk::AccelerationStructureDeviceAddressInfoKHR()
                                 .setAccelerationStructure(handle());
    return m_device->handle().getAccelerationStructureAddressKHR(addressInfo);
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(
    const Device &device, const Allocator &allocator,
    vk::UniqueAccelerationStructureKHR accelerationStructure,
    vk::DeviceAddress deviceAddress, AccelerationStructureBuffer buffer,
    vk::DeviceSize size)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(accelerationStructure))
    , m_deviceAddress(deviceAddress)
    , m_buffer(std::move(buffer))
    , m_size(size) {}

BottomLevelAccelerationStructureBuilder::
    BottomLevelAccelerationStructureBuilder(Device &device,
                                            const Allocator &allocator)
    : m_device(&device)
    , m_allocator(&allocator) {}

BottomLevelAccelerationStructureBuilder &
BottomLevelAccelerationStructureBuilder::add_geometry(const Model::Mesh &mesh) {
    const auto geometry = mesh.acceleration_structure_geometry();
    const auto range_info = mesh.acceleration_structure_range_info();
    m_geometries.push_back(geometry);
    m_range_info.push_back(range_info);
    m_primitive_count.push_back(range_info.primitiveCount);
    return *this;
}

BottomLevelAccelerationStructureBuilder &&
BottomLevelAccelerationStructureBuilder::add_geometries(
    std::span<const Model::Mesh> meshes) {
    for (const auto &mesh : meshes) {
        add_geometry(mesh);
    }
    return std::move(*this);
}

BottomLevelAccelerationStructure
BottomLevelAccelerationStructureBuilder::build() && {
    if (m_geometries.empty()) {
        throw AccelerationStructureCreationException{
            std::source_location::current()};
    }

    // Create acceleration structure geometry info

    auto buildGeometryInfo =
        vk::AccelerationStructureBuildGeometryInfoKHR()
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setFlags(
                vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setGeometries(m_geometries);

    // Get build sizes
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        m_device->handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo,
            m_primitive_count);

    // Create scratch buffer for building
    auto scratchBuffer = m_allocator->create_buffer<ScratchBuffer>(
        buildSizesInfo.buildScratchSize);

    // Create buffer for acceleration structure
    auto buffer = m_allocator->create_buffer<AccelerationStructureBuffer>(
        buildSizesInfo.accelerationStructureSize);

    // Create acceleration structure
    const auto createInfo =
        vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(buffer.handle())
            .setOffset(0)
            .setSize(buildSizesInfo.accelerationStructureSize)
            .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

    auto [result, accelerationStructure] =
        m_device->handle().createAccelerationStructureKHRUnique(createInfo);
    if (result != vk::Result::eSuccess) {
        throw AccelerationStructureCreationException{
            std::source_location::current()};
    }

    // Build acceleration structure
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setSrcAccelerationStructure(VK_NULL_HANDLE)
        .setDstAccelerationStructure(accelerationStructure.get())
        .setGeometries(m_geometries)
        .setScratchData(scratchBuffer.device_address());

    // Create command buffer for building
    auto commandPool = CommandPoolBuilder(*m_device).build();
    auto commandBuffer = commandPool.allocate(1)[0];

    {
        CommandBufferRecorder recorder(commandBuffer);
        recorder.buildAccelerationStructure(buildInfo, m_range_info);
    }
    // Submit command buffer
    m_device->graphicsQueue().enqueue_command_buffer(commandBuffer);
    auto fence = m_device->graphicsQueue().submit({}, {}, {});
    fence.wait();

    return BottomLevelAccelerationStructure(
        *m_device, *m_allocator, std::move(accelerationStructure),
        std::move(buffer), buildSizesInfo.accelerationStructureSize);
}

TopLevelAccelerationStructureBuilder::TopLevelAccelerationStructureBuilder(
    Device &device, const Allocator &allocator)
    : m_device(&device)
    , m_allocator(&allocator) {}

TopLevelAccelerationStructureBuilder &&
TopLevelAccelerationStructureBuilder::add_instance(
    const BottomLevelAccelerationStructure &blas, const glm::mat4 &transform,
    uint32_t instanceId, uint32_t mask, uint32_t hitGroupIndex) {

    auto transform_values = vk::TransformMatrixKHR{}.setMatrix(
        {std::array{1.0f, 0.0f, 0.0f, 0.0f}, std::array{0.0f, 1.0f, 0.0f, 0.0f},
         std::array{0.0f, 0.0f, 1.0f, 0.0f}});

    vk::AccelerationStructureInstanceKHR instance;
    instance.setTransform(transform_values)
        .setInstanceCustomIndex(instanceId)
        .setMask(mask)
        .setInstanceShaderBindingTableRecordOffset(hitGroupIndex)
        .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
        .setAccelerationStructureReference(blas.device_address());

    m_instances.push_back(instance);
    return std::move(*this);
}

TopLevelAccelerationStructureBuilder &
TopLevelAccelerationStructureBuilder::add_instances(
    std::span<const BottomLevelAccelerationStructure> blases,
    std::span<const glm::mat4> transforms) {
    for (size_t i = 0; i < blases.size(); ++i) {
        glm::mat4 transform =
            (i < transforms.size()) ? transforms[i] : glm::mat4(1.0f);
        add_instance(blases[i], transform, static_cast<uint32_t>(i));
    }
    return *this;
}

TopLevelAccelerationStructure TopLevelAccelerationStructureBuilder::build() && {
    if (m_instances.empty()) {
        throw AccelerationStructureCreationException{
            std::source_location::current()};
    }

    // Create instance buffer
    auto instanceBuffer =
        m_allocator->create_buffer<InstanceBuffer>(m_instances.size());

    // Copy instances to buffer
    instanceBuffer.copy(m_instances, 0);

    // Create geometry for top-level acceleration structure
    vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
    instancesData.setData(instanceBuffer.device_address());

    vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry(geometryData)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    // Create build geometry info
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setGeometries(geometry);

    // Get build sizes
    uint32_t instance_count = m_instances.size();
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        m_device->handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo,
            instance_count);

    // Create buffer for acceleration structure
    auto buffer = m_allocator->create_buffer<AccelerationStructureBuffer>(
        buildSizesInfo.accelerationStructureSize);

    // Create acceleration structure
    vk::AccelerationStructureCreateInfoKHR createInfo;
    createInfo.setBuffer(buffer.handle())
        .setOffset(0)
        .setSize(buildSizesInfo.accelerationStructureSize)
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel);

    auto [result, accelerationStructure] =
        m_device->handle().createAccelerationStructureKHRUnique(createInfo);
    if (result != vk::Result::eSuccess) {
        throw AccelerationStructureCreationException{
            std::source_location::current()};
    }

    // Get device address
    vk::AccelerationStructureDeviceAddressInfoKHR addressInfo;
    addressInfo.setAccelerationStructure(accelerationStructure.get());
    vk::DeviceAddress deviceAddress =
        m_device->handle().getAccelerationStructureAddressKHR(addressInfo);

    // Create scratch buffer for building
    auto scratchBuffer = m_allocator->create_buffer<ScratchBuffer>(
        buildSizesInfo.buildScratchSize);

    // Build acceleration structure
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setSrcAccelerationStructure(VK_NULL_HANDLE)
        .setDstAccelerationStructure(accelerationStructure.get())
        .setGeometries(geometry)
        .setScratchData(scratchBuffer.device_address());

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
    buildRangeInfo.setPrimitiveCount(static_cast<uint32_t>(m_instances.size()))
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

    // Create command buffer for building
    auto commandPool = CommandPoolBuilder(*m_device).build();
    auto commandBuffer = commandPool.allocate(1)[0];

    {
        CommandBufferRecorder recorder(commandBuffer);
        recorder.buildAccelerationStructure(buildInfo, {&buildRangeInfo, 1});
    }
    // Submit command buffer
    m_device->graphicsQueue().enqueue_command_buffer(commandBuffer);
    auto fence = m_device->graphicsQueue().submit({}, {}, {});
    fence.wait();

    return TopLevelAccelerationStructure(
        *m_device, *m_allocator, std::move(accelerationStructure),
        deviceAddress, std::move(buffer),
        buildSizesInfo.accelerationStructureSize);
}

} // namespace vw::AccelerationStructure
