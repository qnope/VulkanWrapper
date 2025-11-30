#include "VulkanWrapper/Memory/UniformBufferAllocator.h"
#include "VulkanWrapper/Memory/Allocator.h"

namespace vw {

UniformBufferAllocator::UniformBufferAllocator(const Allocator &allocator,
                                               vk::DeviceSize totalSize,
                                               vk::DeviceSize minAlignment)
    : m_buffer(allocator.create_buffer<std::byte, true, UniformBufferUsage>(totalSize)),
      m_totalSize(totalSize),
      m_minAlignment(minAlignment),
      m_nextIndex(0) {
    
    // Start with one large free block
    m_allocations.push_back({
        .offset = 0,
        .size = totalSize,
        .free = true
    });
}

void UniformBufferAllocator::deallocate(uint32_t index) {
    if (index >= m_allocations.size()) {
        return;
    }

    m_allocations[index].free = true;

    // Merge adjacent free blocks
    // Sort by offset first
    std::sort(m_allocations.begin(), m_allocations.end(),
              [](const Allocation &a, const Allocation &b) {
                  return a.offset < b.offset;
              });

    // Merge adjacent free blocks
    for (size_t i = 0; i < m_allocations.size();) {
        if (!m_allocations[i].free) {
            ++i;
            continue;
        }

        // Look for adjacent free blocks
        size_t j = i + 1;
        while (j < m_allocations.size() && 
               m_allocations[j].free &&
               m_allocations[i].offset + m_allocations[i].size == m_allocations[j].offset) {
            m_allocations[i].size += m_allocations[j].size;
            m_allocations.erase(m_allocations.begin() + j);
        }
        ++i;
    }
}

vk::DeviceSize UniformBufferAllocator::freeSpace() const {
    vk::DeviceSize free = 0;
    for (const auto &alloc : m_allocations) {
        if (alloc.free) {
            free += alloc.size;
        }
    }
    return free;
}

size_t UniformBufferAllocator::allocationCount() const {
    size_t count = 0;
    for (const auto &alloc : m_allocations) {
        if (!alloc.free) {
            ++count;
        }
    }
    return count;
}

void UniformBufferAllocator::clear() {
    m_allocations.clear();
    m_allocations.push_back({
        .offset = 0,
        .size = m_totalSize,
        .free = true
    });
    m_nextIndex = 0;
}

vk::DeviceSize UniformBufferAllocator::align(vk::DeviceSize size) const {
    return (size + m_minAlignment - 1) & ~(m_minAlignment - 1);
}

std::optional<uint32_t> UniformBufferAllocator::findFreeBlock(vk::DeviceSize size) {
    // First-fit strategy
    for (size_t i = 0; i < m_allocations.size(); ++i) {
        if (m_allocations[i].free && m_allocations[i].size >= size) {
            return static_cast<uint32_t>(i);
        }
    }
    return std::nullopt;
}

} // namespace vw
