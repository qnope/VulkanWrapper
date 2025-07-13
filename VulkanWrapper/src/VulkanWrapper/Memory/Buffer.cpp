#include "VulkanWrapper/Memory/Buffer.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
BufferBase::BufferBase(const Device &device, const Allocator &allocator,
                       vk::Buffer buffer, VmaAllocation allocation,
                       VkDeviceSize size)
    : ObjectWithHandle<vk::Buffer>{buffer}
    , m_data{std::make_unique<Data>(&device, &allocator, allocation, size)} {}

VkDeviceSize BufferBase::size_bytes() const noexcept {
    return m_data->m_size_in_bytes;
}

vk::DeviceAddress BufferBase::device_address() const noexcept {
    vk::BufferDeviceAddressInfoKHR addressInfo;
    addressInfo.setBuffer(handle());
    return m_data->m_device->handle().getBufferAddress(addressInfo);
}

void BufferBase::generic_copy(const void *data, VkDeviceSize size,
                              VkDeviceSize offset) {
    vmaCopyMemoryToAllocation(m_data->m_allocator->handle(), data,
                              m_data->m_allocation, offset, size);
}

BufferBase::~BufferBase() {
    if (m_data) {
        vmaDestroyBuffer(m_data->m_allocator->handle(), handle(),
                         m_data->m_allocation);
    }
}

} // namespace vw
