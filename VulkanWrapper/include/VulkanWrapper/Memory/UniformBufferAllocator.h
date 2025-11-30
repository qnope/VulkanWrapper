#pragma once

#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include <vector>
#include <optional>
#include <cstddef>

namespace vw {

/**
 * Represents a chunk of memory allocated from a UniformBufferAllocator.
 */
template<typename T>
struct UniformBufferChunk {
    vk::Buffer handle;      // The underlying buffer handle
    vk::DeviceSize offset;  // Offset within the buffer
    vk::DeviceSize size;    // Size of the chunk in bytes
    uint32_t index;         // Index of this allocation (for tracking)

    /**
     * Returns descriptor buffer info for binding this chunk.
     * Useful for dynamic uniform buffer descriptors.
     */
    vk::DescriptorBufferInfo descriptorInfo() const {
        return vk::DescriptorBufferInfo(handle, offset, size);
    }
};

/**
 * Allocates chunks of uniform buffer memory from a large pre-allocated buffer.
 * Handles alignment requirements and supports deallocation/reuse.
 */
class UniformBufferAllocator {
  public:
    /**
     * Creates a uniform buffer allocator.
     * @param allocator The memory allocator
     * @param totalSize Total size of the backing buffer in bytes
     * @param minAlignment Minimum alignment for allocations (typically 256 for uniform buffers)
     */
    UniformBufferAllocator(const Allocator &allocator, 
                          vk::DeviceSize totalSize,
                          vk::DeviceSize minAlignment = 256);

    /**
     * Allocates a chunk of memory for type T.
     * @param count Number of T elements to allocate space for
     * @return Chunk information, or nullopt if allocation failed
     */
    template<typename T>
    std::optional<UniformBufferChunk<T>> allocate(size_t count = 1);

    /**
     * Deallocates a previously allocated chunk.
     * @param index The index of the chunk to deallocate
     */
    void deallocate(uint32_t index);

    /**
     * Returns the underlying buffer handle.
     */
    [[nodiscard]] vk::Buffer buffer() const { return m_buffer.handle(); }

    /**
     * Returns the total size of the buffer.
     */
    [[nodiscard]] vk::DeviceSize totalSize() const { return m_totalSize; }

    /**
     * Returns the amount of free space remaining.
     */
    [[nodiscard]] vk::DeviceSize freeSpace() const;

    /**
     * Returns the number of active allocations.
     */
    [[nodiscard]] size_t allocationCount() const;

    /**
     * Clears all allocations (marks all as free).
     */
    void clear();

  private:
    struct Allocation {
        vk::DeviceSize offset;
        vk::DeviceSize size;
        bool free;
    };

    Buffer<std::byte, true, UniformBufferUsage> m_buffer;
    vk::DeviceSize m_totalSize;
    vk::DeviceSize m_minAlignment;
    std::vector<Allocation> m_allocations;
    uint32_t m_nextIndex;

    // Aligns a size to the minimum alignment requirement
    vk::DeviceSize align(vk::DeviceSize size) const;

    // Finds a free block that can fit the requested size
    std::optional<uint32_t> findFreeBlock(vk::DeviceSize size);
};

// Template implementation
template<typename T>
std::optional<UniformBufferChunk<T>> UniformBufferAllocator::allocate(size_t count) {
    vk::DeviceSize requestedSize = sizeof(T) * count;
    vk::DeviceSize alignedSize = align(requestedSize);

    auto freeBlockIndex = findFreeBlock(alignedSize);
    if (!freeBlockIndex.has_value()) {
        return std::nullopt;
    }

    uint32_t index = *freeBlockIndex;
    auto &allocation = m_allocations[index];
    
    // If the free block is larger than needed, split it
    if (allocation.size > alignedSize) {
        vk::DeviceSize remainingSize = allocation.size - alignedSize;
        vk::DeviceSize newOffset = allocation.offset + alignedSize;
        
        // Update current allocation
        allocation.size = alignedSize;
        allocation.free = false;
        
        // Create new free allocation for the remainder
        m_allocations.push_back({
            .offset = newOffset,
            .size = remainingSize,
            .free = true
        });
    } else {
        allocation.free = false;
    }

    return UniformBufferChunk<T>{
        .handle = m_buffer.handle(),
        .offset = allocation.offset,
        .size = alignedSize,
        .index = index
    };
}

} // namespace vw
