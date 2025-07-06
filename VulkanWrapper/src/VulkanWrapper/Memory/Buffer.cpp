#include "VulkanWrapper/Memory/Buffer.h"

#include "VulkanWrapper/Memory/Allocator.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {
BufferBase::BufferBase(const Device &device, const Allocator &allocator,
                       vk::Buffer buffer, VmaAllocation allocation,
                       VkDeviceSize size)
    : ObjectWithHandle<vk::Buffer>{buffer}
    , m_device{&device}
    , m_allocator{&allocator}
    , m_allocation{allocation}
    , m_size_in_bytes{size} {}

BufferBase::BufferBase(BufferBase &&other) noexcept
    : ObjectWithHandle<vk::Buffer>{other.handle()}
    , m_device{other.m_device}
    , m_allocator(other.m_allocator)
    , m_allocation{std::exchange(other.m_allocation, {})}
    , m_size_in_bytes{std::exchange(other.m_size_in_bytes, {})} {}

BufferBase &BufferBase::operator=(BufferBase &&other) noexcept {
    static_cast<ObjectWithHandle<vk::Buffer> &>(*this) = other;
    m_device = other.m_device;
    other.m_allocation = std::exchange(other.m_allocation, {});
    other.m_size_in_bytes = std::exchange(other.m_size_in_bytes, {});
    return *this;
}

VkDeviceSize BufferBase::size_bytes() const noexcept { return m_size_in_bytes; }

vk::DeviceAddress BufferBase::device_address() const noexcept {
    vk::BufferDeviceAddressInfoKHR addressInfo;
    addressInfo.setBuffer(handle());
    return m_device->handle().getBufferAddress(addressInfo);
}

void BufferBase::generic_copy(const void *data, VkDeviceSize size,
                              VkDeviceSize offset) {
    vmaCopyMemoryToAllocation(m_allocator->handle(), data, m_allocation, offset,
                              size);
}

BufferBase::~BufferBase() {
    if (m_allocation != VmaAllocation()) {
        vmaDestroyBuffer(m_allocator->handle(), handle(), m_allocation);
    }
}

} // namespace vw
