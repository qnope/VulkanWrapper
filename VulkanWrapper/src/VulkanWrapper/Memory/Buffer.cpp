#include "VulkanWrapper/Memory/Buffer.h"

#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {
BufferBase::BufferBase(Allocator &allocator, vk::Buffer buffer,
                       VmaAllocation allocation)
    : ObjectWithHandle<vk::Buffer>{buffer}
    , m_allocator{allocator}
    , m_allocation{allocation} {}

BufferBase::BufferBase(BufferBase &&other) noexcept
    : ObjectWithHandle<vk::Buffer>{other.handle()}
    , m_allocator(other.m_allocator)
    , m_allocation{std::exchange(other.m_allocation, {})} {}

BufferBase &BufferBase::operator=(BufferBase &&other) noexcept {
    static_cast<ObjectWithHandle<vk::Buffer> &>(*this) = other;
    other.m_allocation = std::exchange(other.m_allocation, {});
    return *this;
}

void BufferBase::generic_copy(const void *data, uint64_t size) {
    vmaCopyMemoryToAllocation(m_allocator.handle(), data, m_allocation, 0,
                              size);
}

BufferBase::~BufferBase() {
    if (m_allocation != VmaAllocation())
        vmaDestroyBuffer(m_allocator.handle(), handle(), m_allocation);
}

} // namespace vw
