#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw::rt::as {
class BottomLevel
    : public ObjectWithUniqueHandle<vk::UniqueAccelerationStructureKHR> {
  public:
  private:
};

/*
 *         vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

     vertexBufferDeviceAddress.deviceAddress =
         getBufferDeviceAddress(vertexBuffer);
     indexBufferDeviceAddress.deviceAddress =
         getBufferDeviceAddress(indexBuffer);

     // Build
     vk::AccelerationStructureGeometryKHR accelerationStructureGeometry{};
     accelerationStructureGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
     accelerationStructureGeometry.geometryType =
         vk::GeometryTypeKHR::eTriangles;
     accelerationStructureGeometry.geometry.triangles.vertexFormat =
         vk::Format::eR32G32B32Sfloat;
     accelerationStructureGeometry.geometry.triangles.vertexData =
         vertexBufferDeviceAddress;
     accelerationStructureGeometry.geometry.triangles.maxVertex = 2;
     accelerationStructureGeometry.geometry.triangles.vertexStride =
         sizeof(vw::Vertex3D);
     accelerationStructureGeometry.geometry.triangles.indexType =
         vk::IndexType::eUint32;
     accelerationStructureGeometry.geometry.triangles.indexData =
         indexBufferDeviceAddress;

     // Get size info
     vk::AccelerationStructureBuildGeometryInfoKHR
         accelerationStructureBuildGeometryInfo{};
     accelerationStructureBuildGeometryInfo.type =
         vk::AccelerationStructureTypeKHR::eBottomLevel;
     accelerationStructureBuildGeometryInfo.flags =
         vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
     accelerationStructureBuildGeometryInfo.setGeometries(
         accelerationStructureGeometry);

     const uint32_t numTriangles = 1;

     auto accelerationStructureBuildSizesInfo =
         device.handle().getAccelerationStructureBuildSizesKHR(
             vk::AccelerationStructureBuildTypeKHR::eDevice,
             accelerationStructureBuildGeometryInfo, numTriangles);

     createAccelerationStructureBuffer(bottomLevelAS,
                                       accelerationStructureBuildSizesInfo);

     vk::AccelerationStructureCreateInfoKHR
         accelerationStructureCreateInfo{};

     accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer->handle();
     accelerationStructureCreateInfo.size =
         accelerationStructureBuildSizesInfo.accelerationStructureSize;
     accelerationStructureCreateInfo.type =
         vk::AccelerationStructureTypeKHR::eBottomLevel;

     bottomLevelAS.handle = device.handle()
                                .createAccelerationStructureKHRUnique(
                                    accelerationStructureCreateInfo)
                                .value;
*/

class BottomLevelBuilder {
  public:
    BottomLevelBuilder(const Device &device, const Allocator &allocator);

  private:
    const Device &device;
    const Allocator &allocator;
};
} // namespace vw::rt::as
