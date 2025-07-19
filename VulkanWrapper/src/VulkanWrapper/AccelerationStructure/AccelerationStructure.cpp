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
    vk::DeviceAddress deviceAddress, AccelerationStructureBuffer buffer,
    vk::DeviceSize size)
    : ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR>(
          std::move(accelerationStructure))
    , m_deviceAddress(deviceAddress)
    , m_buffer(std::move(buffer))
    , m_size(size) {}

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
    auto geometry = mesh.acceleration_structure_geometry();
    m_geometries.push_back(geometry);
    m_maxPrimitiveCounts.push_back(mesh.index_count() /
                                   3); // Assuming triangles
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

    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setGeometries(m_geometries);

    // Get build sizes
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        m_device->handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo,
            m_maxPrimitiveCounts);

    // Create buffer for acceleration structure
    auto buffer = m_allocator->create_buffer<AccelerationStructureBuffer>(
        buildSizesInfo.accelerationStructureSize);

    // Create acceleration structure
    vk::AccelerationStructureCreateInfoKHR createInfo;
    createInfo.setBuffer(buffer.handle())
        .setOffset(0)
        .setSize(buildSizesInfo.accelerationStructureSize)
        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

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
    buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setSrcAccelerationStructure(VK_NULL_HANDLE)
        .setDstAccelerationStructure(accelerationStructure.get())
        .setGeometries(m_geometries)
        .setScratchData(
            vk::DeviceOrHostAddressKHR{scratchBuffer.device_address()});

    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
    for (auto primitiveCount : m_maxPrimitiveCounts) {
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo;
        buildRangeInfo.setPrimitiveCount(primitiveCount)
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        buildRangeInfos.push_back(buildRangeInfo);
    }

    // Create command buffer for building
    auto commandPool = CommandPoolBuilder(*m_device).build();
    auto commandBuffer = commandPool.allocate(1)[0];

    {
        CommandBufferRecorder recorder(commandBuffer);
        recorder.buildAccelerationStructure(buildInfo, buildRangeInfos);
    }
    // Submit command buffer
    m_device->graphicsQueue().enqueue_command_buffer(commandBuffer);
    auto fence = m_device->graphicsQueue().submit({}, {}, {});
    fence.wait();

    return BottomLevelAccelerationStructure(
        *m_device, *m_allocator, std::move(accelerationStructure),
        deviceAddress, std::move(buffer),
        buildSizesInfo.accelerationStructureSize);
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
        {std::array{transform[0][0], transform[1][0], transform[2][0],
                    transform[3][0]},
         std::array{transform[0][1], transform[1][1], transform[2][1],
                    transform[3][1]},
         std::array{transform[0][2], transform[1][2], transform[2][2],
                    transform[3][2]}});

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
    instancesData.setArrayOfPointers(VK_FALSE).setData(
        vk::DeviceOrHostAddressConstKHR{instanceBuffer.device_address()});

    vk::AccelerationStructureGeometryDataKHR geometryData;
    geometryData.setInstances(instancesData);

    vk::AccelerationStructureGeometryKHR geometry;
    geometry.setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry(geometryData)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    // Create build geometry info
    vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
    buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setGeometryCount(1)
        .setPGeometries(&geometry);

    // Get build sizes
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
        m_device->handle().getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo,
            {static_cast<uint32_t>(m_instances.size())});

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
        .setGeometryCount(1)
        .setPGeometries(&geometry)
        .setScratchData(
            vk::DeviceOrHostAddressKHR{scratchBuffer.device_address()});

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
