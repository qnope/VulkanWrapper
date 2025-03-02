#include "VulkanWrapper/Memory/Buffer.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {
BufferBase::BufferBase(Allocator &allocator, vk::Buffer buffer,
                       VmaAllocation allocation, VkDeviceSize size)
    : ObjectWithHandle<vk::Buffer>{buffer}
    , m_allocator{allocator}
    , m_allocation{allocation}
    , m_size_in_bytes{size} {}

BufferBase::BufferBase(BufferBase &&other) noexcept
    : ObjectWithHandle<vk::Buffer>{other.handle()}
    , m_allocator(other.m_allocator)
    , m_allocation{std::exchange(other.m_allocation, {})} {}

BufferBase &BufferBase::operator=(BufferBase &&other) noexcept {
    static_cast<ObjectWithHandle<vk::Buffer> &>(*this) = other;
    other.m_allocation = std::exchange(other.m_allocation, {});
    return *this;
}

VkDeviceSize BufferBase::size_in_bytes() const noexcept {
    return m_size_in_bytes;
}

void BufferBase::generic_copy(const void *data, VkDeviceSize size,
                              VkDeviceSize offset) {
    vmaCopyMemoryToAllocation(m_allocator.handle(), data, m_allocation, offset,
                              size);
}

BufferBase::~BufferBase() {
    if (m_allocation != VmaAllocation())
        vmaDestroyBuffer(m_allocator.handle(), handle(), m_allocation);
}

} // namespace vw
