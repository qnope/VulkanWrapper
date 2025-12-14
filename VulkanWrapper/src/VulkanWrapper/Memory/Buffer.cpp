#include "VulkanWrapper/Memory/Buffer.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
BufferBase::BufferBase(std::shared_ptr<const Device> device,
                       std::shared_ptr<const Allocator> allocator,
                       vk::Buffer buffer, VmaAllocation allocation,
                       VkDeviceSize size)
    : ObjectWithHandle<vk::Buffer>{buffer}
    , m_data{std::make_unique<Data>(std::move(device), std::move(allocator),
                                    allocation, size)} {}

VkDeviceSize BufferBase::size_bytes() const noexcept {
    return m_data->m_size_in_bytes;
}

vk::DeviceAddress BufferBase::device_address() const {
    vk::BufferDeviceAddressInfoKHR addressInfo;
    addressInfo.setBuffer(handle());
    auto result = m_data->m_device->handle().getBufferAddress(addressInfo);
    if (result == 0) {
        throw LogicException::invalid_state(
            "Buffer device address is 0 - buffer may not have been created "
            "with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT");
    }
    return result;
}

void BufferBase::write_bytes(const void *data, VkDeviceSize size,
                             VkDeviceSize offset) {
    if (!m_data || !m_data->m_allocation) {
        throw LogicException::invalid_state("Buffer has invalid allocation");
    }
    if (offset + size > m_data->m_size_in_bytes) {
        throw LogicException::out_of_range("buffer copy", offset + size,
                                           m_data->m_size_in_bytes);
    }
    check_vma(vmaCopyMemoryToAllocation(m_data->m_allocator->handle(), data,
                                        m_data->m_allocation, offset, size),
              "Failed to copy memory to allocation");
}

std::vector<std::byte> BufferBase::read_bytes(VkDeviceSize offset, VkDeviceSize size) const {
    std::vector<std::byte> result(size);
    check_vma(vmaCopyAllocationToMemory(m_data->m_allocator->handle(),
                                        m_data->m_allocation, offset,
                                        result.data(), size),
              "Failed to copy memory from allocation");
    return result;
}

BufferBase::~BufferBase() {
    if (m_data) {
        vmaDestroyBuffer(m_data->m_allocator->handle(), handle(),
                         m_data->m_allocation);
    }
}

} // namespace vw
