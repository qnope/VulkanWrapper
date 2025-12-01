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
    auto result = m_data->m_device->handle().getBufferAddress(addressInfo);
    assert(result != 0);
    return result;
}

void BufferBase::generic_copy(const void *data, VkDeviceSize size,
                              VkDeviceSize offset) {
    if (!m_data || !m_data->m_allocation) {
        throw std::runtime_error("Invalid allocation");
    }
    if (offset + size > m_data->m_size_in_bytes) {
        throw std::runtime_error("Copy would exceed buffer size");
    }
    VkResult res = vmaCopyMemoryToAllocation(
        m_data->m_allocator->handle(),
        data,
        m_data->m_allocation,
        offset,
        size);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to copy memory to allocation");
    }
}

std::vector<std::byte> BufferBase::generic_as_vector(VkDeviceSize offset, VkDeviceSize size) const {
    std::vector<std::byte> result(size);
    VkResult res = vmaCopyAllocationToMemory(
        m_data->m_allocator->handle(),
        m_data->m_allocation,
        offset,
        result.data(),
        size);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to copy memory from allocation");
    }
    
    return result;
}

BufferBase::~BufferBase() {
    if (m_data) {
        vmaDestroyBuffer(m_data->m_allocator->handle(), handle(),
                         m_data->m_allocation);
    }
}

} // namespace vw
